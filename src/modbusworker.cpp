#include "modbusworker.h"
#include "modbusdevice.h"
#include "commmonitor.h"
#include "modbuscodec.h"
#include "modbusdefs.h"
#include <QTimer>
#include <QVector>
#include <algorithm>

ModbusWorker::ModbusWorker(QObject *parent)
    : QObject(parent)
{
    m_device  = new ModbusDevice(this);
    m_monitor = new CommMonitor(this);
    // 设备帧/失败事件 -> 监控器（同线程直连）
    connect(m_device, &ModbusDevice::frameSent, m_monitor, &CommMonitor::onFrameSent);
    connect(m_device, &ModbusDevice::frameReceived, m_monitor, &CommMonitor::onFrameReceived);
    connect(m_device, &ModbusDevice::operationFailed, m_monitor, &CommMonitor::onOperationError);
}

ModbusWorker::~ModbusWorker() = default;

void ModbusWorker::setConfig(const ModbusConfig &cfg)
{
    QMutexLocker lock(&m_mutex);
    m_cfg = cfg;
    if (m_timer)
        m_timer->setInterval(qMax(50, m_cfg.pollInterval));
}

void ModbusWorker::applyConfig(const ModbusConfig &cfg)
{
    QMutexLocker lock(&m_mutex);
    // 仅热更新协议层与轮询层；传输层参数在连接期间不变
    m_cfg.protocol        = cfg.protocol;
    m_cfg.slave           = cfg.slave;
    m_cfg.responseTimeout = cfg.responseTimeout;
    m_cfg.pollInterval    = cfg.pollInterval;
    m_cfg.readMode        = cfg.readMode;
    m_cfg.coilWriteFunc   = cfg.coilWriteFunc;
    m_cfg.regWriteFunc    = cfg.regWriteFunc;

    m_device->setConfig(m_cfg);
    if (m_timer)
        m_timer->setInterval(qMax(50, m_cfg.pollInterval));
    if (m_device->isConnected())
        m_monitor->reportInfo("协议配置已更新");
}

void ModbusWorker::connectDevice(ModbusConfig *cfg)
{
    QMutexLocker lock(&m_mutex);
    m_cfg = *cfg;
    m_monitor->resetStats();

    if (!m_device->connectDevice(m_cfg))
    {
        QString e = m_device->errorString();
        emit connectionStateChanged(false);
        emit connectError(e);
        m_monitor->reportError("连接失败: " + e);
        return;
    }
    if (!m_timer)
    {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &ModbusWorker::doPoll);
    }
    m_timer->setInterval(qMax(50, m_cfg.pollInterval));
    m_timer->start();
    emit connectionStateChanged(true);
    m_monitor->reportInfo("连接成功");
}

void ModbusWorker::disconnectDevice()
{
    QMutexLocker lock(&m_mutex);
    if (m_timer) m_timer->stop();
    const bool wasConnected = m_device->isConnected();
    m_device->disconnectDevice();
    emit connectionStateChanged(false);
    if (wasConnected)
        m_monitor->reportInfo("已断开连接");
}

void ModbusWorker::setAreaPlan(int areaIndex, QList<RegPlanItem> *items)
{
    if (areaIndex < 0 || areaIndex > 3) return;
    QMutexLocker lock(&m_mutex);
    m_plans[areaIndex] = *items;
    delete items;
}

void ModbusWorker::doPoll()
{
    QMutexLocker lock(&m_mutex);
    if (!m_device->isConnected())
        return;
    for (int a = 0; a < 4; ++a)
    {
        if (!m_plans[a].isEmpty())
            runReadArea(a, m_plans[a]);
    }
}

void ModbusWorker::runReadArea(int areaIndex, const QList<RegPlanItem> &items)
{
    const AreaInfo &info = areaInfo(areaIndex);
    int maxQ = (info.readFunc == 1 || info.readFunc == 2) ? 2000 : 125;

    // 计划内地址升序排列，供连续段合并使用
    QList<int> addrs;
    for (const RegPlanItem &it : items)
        addrs.append(it.address);
    std::sort(addrs.begin(), addrs.end());

    if (m_cfg.readMode)
    {
        // 批量模式：计划内连续地址合并为一次读取，非连续地址单独读取；
        // 连续段超过单次读取上限时分片
        int i = 0;
        while (i < addrs.size())
        {
            int runStart = addrs[i];
            int runEnd   = runStart;
            while (i + 1 < addrs.size() && addrs[i + 1] == addrs[i] + 1)
                runEnd = addrs[++i];
            ++i;
            for (int s = runStart; s <= runEnd; s += maxQ)
                readChunk(areaIndex, s, qMin(maxQ, runEnd - s + 1), items);
        }
    }
    else
    {
        // 单点模式：计划内地址逐个读取
        for (int addr : addrs)
            readChunk(areaIndex, addr, 1, items);
    }
}

void ModbusWorker::readChunk(int areaIndex, int start, int cnt,
                             const QList<RegPlanItem> &items)
{
    const AreaInfo &info = areaInfo(areaIndex);
    const bool bitArea = (info.readFunc == 1 || info.readFunc == 2);

    QString err;
    quint8 mbErr = 0;
    QVector<qint64> values;
    bool ok;
    if (bitArea)
    {
        QVector<bool> bits;
        ok = m_device->readBits(info.readFunc, start, cnt, bits, err, &mbErr);
        for (bool b : bits)
            values.append(b ? 1 : 0);
    }
    else
    {
        QVector<quint16> regs;
        ok = m_device->readRegisters(info.readFunc, start, cnt, regs, err, &mbErr);
        for (quint16 r : regs)
            values.append(r);
    }

    if (!ok)
    {
        // 区分超时 / 设备异常 / 其它无效；只标记本分片覆盖的地址，
        // 其它分片各自独立处理，避免连累通信正常的寄存器
        int status = ReadInvalid;
        qint64 errValue = 0;
        QString errText = err;
        if (mbErr != 0)
        {
            status   = ReadError;
            errValue = mbErr;
            errText  = ModbusCodec::exceptionText(mbErr);
        }
        else if (err.contains("超时"))
        {
            status = ReadTimeout;
        }
        for (const RegPlanItem &it : items)
        {
            if (it.address >= start && it.address < start + cnt)
                emit readResult(areaIndex, it.address, status, 0, errText, errValue);
        }
        return;
    }

    // 仅处理本分片所覆盖的地址，其它地址由覆盖它的分片负责
    for (const RegPlanItem &it : items)
    {
        if (it.address < start || it.address >= start + cnt)
            continue;
        int idx = it.address - start;
        if (idx < values.size())
        {
            qint64 v = values[idx];
            if (it.type == TypeS16)
                v = (qint16)(quint16)v;
            emit readResult(areaIndex, it.address, ReadOk, v, QString(), 0);
        }
        else
        {
            emit readResult(areaIndex, it.address, ReadInvalid, 0, QString(), 0);
        }
    }
}

void ModbusWorker::writeRegister(int areaIndex, int address, DataType type, qint64 value)
{
    QMutexLocker lock(&m_mutex);
    if (!m_device->isConnected())
    {
        emit writeResult(areaIndex, address, false, "未连接");
        m_monitor->reportError("未连接");
        return;
    }
    Q_UNUSED(type);

    QString err;
    bool ok = (areaIndex == 0)
              ? m_device->writeCoil(address, value != 0, m_cfg.coilWriteFunc, err)
              : m_device->writeRegister(address, (quint16)value, m_cfg.regWriteFunc, err);
    emit writeResult(areaIndex, address, ok, ok ? QString() : err);
}
