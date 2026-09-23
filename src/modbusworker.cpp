#include "modbusworker.h"
#include "modbusdevice.h"
#include "commmonitor.h"
#include "modbuscodec.h"
#include "modbusdefs.h"
#include <QTimer>
#include <QVector>
#include <algorithm>

// 该地址是否为某 32 位点位的首寄存器（用于避免分片把一对寄存器切开）
static bool isPairBase(const QList<RegPlanItem> &items, int addr)
{
    for (const RegPlanItem &it : items)
        if (it.address == addr && is32BitType(it.type))
            return true;
    return false;
}

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

    // 收集计划覆盖的全部寄存器地址（32 位点位占两个），升序去重后供连续段合并
    QList<int> addrs;
    for (const RegPlanItem &it : items)
    {
        addrs.append(it.address);
        if (is32BitType(it.type))
            addrs.append(it.address + 1);
    }
    std::sort(addrs.begin(), addrs.end());
    addrs.erase(std::unique(addrs.begin(), addrs.end()), addrs.end());

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
            {
                int cnt = qMin(maxQ, runEnd - s + 1);
                // 若分片末尾恰为某 32 位点位的首寄存器，则多读一个，避免其配偶落入下一分片
                if (s + cnt <= runEnd && isPairBase(items, s + cnt - 1))
                    ++cnt;
                readChunk(areaIndex, s, cnt, items);
            }
        }
    }
    else
    {
        // 单点模式：计划内每个点位单独读取（32位点位一次读两个寄存器）
        for (const RegPlanItem &it : items)
            readChunk(areaIndex, it.address, regCountOf(it.type), items);
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
        if (is32BitType(it.type))
        {
            if (idx + 1 < values.size() && it.address + 1 < start + cnt)
            {
                quint32 bits = decode32((quint16)values[idx],
                                        (quint16)values[idx + 1], it.byteOrder);
                // F32 以比特模式存入 value，展示侧按 float 重新解释
                qint64 v = (it.type == TypeS32) ? (qint32)bits : (qint64)bits;
                emit readResult(areaIndex, it.address, ReadOk, v, QString(), 0);
            }
            else
            {
                emit readResult(areaIndex, it.address, ReadInvalid, 0, QString(), 0);
            }
        }
        else if (idx < values.size())
        {
            qint64 v = values[idx];
            // 字节序换算后再按符号解释：AB=原样，BA=交换两字节
            if (it.byteOrder == ByteOrderBA)
                v = swapBytes16((quint16)v);
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

void ModbusWorker::writeRegister(int areaIndex, int address, DataType type,
                                 ByteOrder byteOrder, qint64 value)
{
    QMutexLocker lock(&m_mutex);
    if (!m_device->isConnected())
    {
        emit writeResult(areaIndex, address, false, "未连接");
        m_monitor->reportError("未连接");
        return;
    }
    QString err;
    bool ok;
    if (areaIndex == 0)
    {
        // 遥控点位：线圈写入
        ok = m_device->writeCoil(address, value != 0, m_cfg.coilWriteFunc, err);
    }
    else if (is32BitType(type))
    {
        // 32 位类型固定用功能码 16 写两个连续寄存器
        quint16 w0 = 0, w1 = 0;
        encode32((quint32)value, byteOrder, w0, w1);
        QVector<quint16> regs;
        regs << w0 << w1;
        ok = m_device->writeRegisters(address, regs, err);
    }
    else
    {
        // 写入前按字节序反向换算，与读取侧对称
        quint16 out = (quint16)value;
        if (byteOrder == ByteOrderBA)
            out = swapBytes16(out);
        ok = m_device->writeRegister(address, out, m_cfg.regWriteFunc, err);
    }
    emit writeResult(areaIndex, address, ok, ok ? QString() : err);
}
