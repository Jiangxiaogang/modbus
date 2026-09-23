#include "modbusworker.h"
#include "modbusdevice.h"
#include "modbuscodec.h"
#include "modbusdefs.h"
#include <QTimer>
#include <QVector>
#include <algorithm>

static bool isPairBase(const QList<RegPlanItem> &items, int addr)
{
    for (const RegPlanItem &it : items)
        if (it.address == addr && is32BitType(it.type))
            return true;
    return false;
}

ModbusWorker::ModbusWorker(ModbusDevice *device, const ModbusConfig *cfg, QObject *parent)
    : QObject(parent), m_cfgPtr(cfg), m_device(device)
{
}

ModbusWorker::~ModbusWorker() = default;

void ModbusWorker::applyConfig()
{
    QMetaObject::invokeMethod(this, [this]{ doApplyConfig(); }, Qt::QueuedConnection);
}

void ModbusWorker::connectDevice()
{
    QMetaObject::invokeMethod(this, [this]{ doConnectDevice(); }, Qt::QueuedConnection);
}

void ModbusWorker::disconnectDevice()
{
    QMetaObject::invokeMethod(this, [this]{ doDisconnectDevice(); }, Qt::QueuedConnection);
}

void ModbusWorker::setAreaPlan(int areaIndex, const QList<RegPlanItem> &items)
{
    QMetaObject::invokeMethod(this, [this, areaIndex, items]{ doSetAreaPlan(areaIndex, items); },
                              Qt::QueuedConnection);
}

void ModbusWorker::writeRegister(int areaIndex, int address, DataType type,
                                 ByteOrder byteOrder, qint64 value)
{
    QMetaObject::invokeMethod(this, [this, areaIndex, address, type, byteOrder, value]{
        doWriteRegister(areaIndex, address, type, byteOrder, value);
    }, Qt::QueuedConnection);
}

void ModbusWorker::doApplyConfig()
{
    QMutexLocker lock(&m_mutex);

    m_cfg.protocol        = m_cfgPtr->protocol;
    m_cfg.slave           = m_cfgPtr->slave;
    m_cfg.responseTimeout = m_cfgPtr->responseTimeout;
    m_cfg.pollInterval    = m_cfgPtr->pollInterval;
    m_cfg.readMode        = m_cfgPtr->readMode;
    m_cfg.coilWriteFunc   = m_cfgPtr->coilWriteFunc;
    m_cfg.regWriteFunc    = m_cfgPtr->regWriteFunc;

    m_device->setConfig(m_cfg);
    if (m_timer)
        m_timer->setInterval(qMax(50, m_cfg.pollInterval));
    if (m_device->isConnected())
        emit infoMessage("协议配置已更新");
}

void ModbusWorker::doConnectDevice()
{
    QMutexLocker lock(&m_mutex);
    m_cfg = *m_cfgPtr;

    if (!m_device->connectDevice(m_cfg))
    {
        QString e = m_device->errorString();
        emit connectionStateChanged(false);
        emit connectError(e);
        emit errorMessage("连接失败: " + e);
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
    emit infoMessage("连接成功");
}

void ModbusWorker::doDisconnectDevice()
{
    QMutexLocker lock(&m_mutex);
    if (m_timer) m_timer->stop();
    const bool wasConnected = m_device->isConnected();
    m_device->disconnectDevice();
    emit connectionStateChanged(false);
    if (wasConnected)
        emit infoMessage("已断开连接");
}

void ModbusWorker::doSetAreaPlan(int areaIndex, const QList<RegPlanItem> &items)
{
    if (areaIndex < 0 || areaIndex > 3) return;
    QMutexLocker lock(&m_mutex);
    m_plans[areaIndex] = items;
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

                if (s + cnt <= runEnd && isPairBase(items, s + cnt - 1))
                    ++cnt;
                readChunk(areaIndex, s, cnt, items);
            }
        }
    }
    else
    {

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

void ModbusWorker::doWriteRegister(int areaIndex, int address, DataType type,
                                   ByteOrder byteOrder, qint64 value)
{
    QMutexLocker lock(&m_mutex);
    if (!m_device->isConnected())
    {
        emit writeResult(areaIndex, address, false, "未连接");
        emit errorMessage("未连接");
        return;
    }
    QString err;
    bool ok;
    if (areaIndex == 0)
    {

        ok = m_device->writeCoil(address, value != 0, m_cfg.coilWriteFunc, err);
    }
    else if (is32BitType(type))
    {

        quint16 w0 = 0, w1 = 0;
        encode32((quint32)value, byteOrder, w0, w1);
        QVector<quint16> regs;
        regs << w0 << w1;
        ok = m_device->writeRegisters(address, regs, err);
    }
    else
    {

        quint16 out = (quint16)value;
        if (byteOrder == ByteOrderBA)
            out = swapBytes16(out);
        ok = m_device->writeRegister(address, out, m_cfg.regWriteFunc, err);
    }
    emit writeResult(areaIndex, address, ok, ok ? QString() : err);
}
