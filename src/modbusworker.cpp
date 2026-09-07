#include "modbusworker.h"
#include "modbusclient.h"
#include "modbuscodec.h"
#include "modbusdefs.h"
#include <QTimer>
#include <QTime>
#include <QMap>

ModbusWorker::ModbusWorker(QObject *parent)
    : QObject(parent)
    , m_client(0)
    , m_timer(0)
    , m_tx(0)
    , m_rx(0)
    , m_errCount(0)
    , m_logIsWrite(false)
{
}

ModbusWorker::~ModbusWorker()
{
    if (m_client)
    {
        m_client->close();
        delete m_client;
    }
}

void ModbusWorker::setConfig(const ModbusConfig &cfg)
{
    QMutexLocker lock(&m_mutex);
    m_cfg = cfg;
    if (m_timer && m_client)
        m_timer->setInterval(qMax(50, m_cfg.pollInterval));
}

void ModbusWorker::connectDevice(ModbusConfig *cfg)
{
    QMutexLocker lock(&m_mutex);
    m_cfg = *cfg;
    if (m_client)
    {
        m_client->close();
        delete m_client;
        m_client = 0;
    }
    m_client = new ModbusClient(this);
    m_tx = m_rx = m_errCount = 0;
    // 帧级收发 -> 通信日志（同线程直连，带读/写标签转发）
    connect(m_client, SIGNAL(frameSent(QByteArray)), this, SLOT(onFrameSent(QByteArray)));
    connect(m_client, SIGNAL(frameReceived(QByteArray)), this, SLOT(onFrameReceived(QByteArray)));

    if (!m_client->open(m_cfg))
    {
        QString e = m_client->errorString();
        delete m_client;
        m_client = 0;
        emit connectionStateChanged(false);
        emit connectError(e);
        emit statsUpdated(0, 0, 0);
        emit logError(QTime::currentTime().toString("hh:mm:ss.zzz"),
                      "连接失败: " + e);
        return;
    }
    if (!m_timer)
    {
        m_timer = new QTimer(this);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(doPoll()));
    }
    m_timer->setInterval(qMax(50, m_cfg.pollInterval));
    m_timer->start();
    emit connectionStateChanged(true);
    emit statsUpdated(0, 0, 0);
    emit logInfo(QTime::currentTime().toString("hh:mm:ss.zzz"), "连接成功");
}

void ModbusWorker::disconnectDevice()
{
    QMutexLocker lock(&m_mutex);
    if (m_timer) m_timer->stop();
    const bool wasConnected = (m_client != 0);
    if (m_client)
    {
        m_client->close();
        delete m_client;
        m_client = 0;
    }
    emit connectionStateChanged(false);
    if (wasConnected)
        emit logInfo(QTime::currentTime().toString("hh:mm:ss.zzz"), "已断开连接");
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
    if (!m_client || !m_client->isOpen())
        return;
    for (int a = 0; a < 4; ++a)
    {
        if (m_plans[a].isEmpty())
            continue;
        runReadArea(a, m_plans[a]);
    }
    emit statsUpdated(m_tx, m_rx, m_errCount);
}

void ModbusWorker::runReadArea(int areaIndex, const QList<RegPlanItem> &items)
{
    m_logIsWrite = false; // 读事务，供日志标签区分
    const AreaInfo &info = areaInfo(areaIndex);
    int maxQ = (info.readFunc == 1 || info.readFunc == 2) ? 2000 : 125;
    int q = m_cfg.readMode ? maxQ : 1;

    int minA = items.first().address;
    int maxA = items.first().address;
    foreach (const RegPlanItem &it, items)
    {
        if (it.address < minA) minA = it.address;
        if (it.address > maxA) maxA = it.address;
    }

    int start = minA;
    while (start <= maxA)
    {
        int cnt = qMin(q, maxA - start + 1);
        QByteArray tx;
        tx.append((char)((start >> 8) & 0xFF));
        tx.append((char)(start & 0xFF));
        tx.append((char)((cnt >> 8) & 0xFF));
        tx.append((char)(cnt & 0xFF));

        QByteArray rx;
        QString err;
        qint64 txB = 0, rxB = 0;
        quint8 mbErr = 0;
        bool ok = m_client->transact((quint8)info.readFunc, tx, rx, err, &txB, &rxB, &mbErr);
        m_tx += (quint32)txB;
        if (ok) m_rx += (quint32)rxB;
        else m_errCount++;

        if (!ok)
        {
            // 区分超时 / 设备异常 / 其它无效，给整个区统一标记
            int status;
            qint64 errValue = 0;
            QString errText;
            if (mbErr != 0)
            {
                status   = ReadError;
                errValue = mbErr;
                errText  = ModbusCodec::exceptionText(mbErr);
            }
            else if (err == "响应超时")
            {
                status  = ReadTimeout;
                errText = err;
            }
            else
            {
                status  = ReadInvalid;
                errText = err;
            }
            emit logError(QTime::currentTime().toString("hh:mm:ss.zzz"), errText);
            foreach (const RegPlanItem &it, items)
                emit readResult(areaIndex, it.address, status, 0, errText, errValue);
            return;
        }

        // 解析响应，建立地址->值 映射
        QMap<int, qint64> values;
        if (rx.size() < 1)
        {
            continue;
        }
        int byteCount = (quint8)rx[0];
        const char *data = rx.constData() + 1;
        if (info.readFunc == 1 || info.readFunc == 2)
        {
            for (int i = 0; i < cnt; ++i)
            {
                int addr = start + i;
                int bit = (data[i / 8] >> (i % 8)) & 0x01;
                values[addr] = bit;
            }
        }
        else
        {
            int n = byteCount / 2;
            for (int i = 0; i < cnt && i < n; ++i)
            {
                int addr = start + i;
                int hi = (quint8)data[2 * i];
                int lo = (quint8)data[2 * i + 1];
                quint16 word = (quint16)((hi << 8) | lo);
                // 默认按 U16；S16 在视图侧由类型解析，这里统一给出无符号值
                values[addr] = word;
            }
        }
        // 仅处理本分片所覆盖的地址，避免被后续分片误判为 ReadInvalid 而覆盖
        const int chunkEnd = start + cnt;
        foreach (const RegPlanItem &it, items)
        {
            if (it.address < start || it.address >= chunkEnd)
                continue;  // 由覆盖该地址的其它分片负责

            QMap<int, qint64>::const_iterator itv = values.find(it.address);
            if (itv != values.end())
            {
                qint64 v = itv.value();
                if (it.type == TypeS16)
                {
                    qint16 s = (qint16)(quint16)v;
                    v = s;
                }
                emit readResult(areaIndex, it.address, ReadOk, v, QString(), 0);
            }
            else
            {
                emit readResult(areaIndex, it.address, ReadInvalid, 0, QString(), 0);
            }
        }
        start += cnt;
    }
}

void ModbusWorker::writeRegister(int areaIndex, int address, DataType type, qint64 value)
{
    QMutexLocker lock(&m_mutex);
    m_logIsWrite = true; // 写事务，供日志标签区分
    if (!m_client || !m_client->isOpen())
    {
        emit writeResult(areaIndex, address, false, "未连接");
        emit logError(QTime::currentTime().toString("hh:mm:ss.zzz"), "未连接");
        return;
    }
    const AreaInfo &info = areaInfo(areaIndex);
    Q_UNUSED(info);
    QByteArray tx;
    quint8 func = 0;
    QString err;
    qint64 txB = 0, rxB = 0;
    bool ok = false;

    if (areaIndex == 0)                   // 线圈
    {
        func = (quint8)m_cfg.coilWriteFunc;
        if (func == 15)
        {
            quint8 b = value ? 0x01 : 0x00;
            tx.append((char)((address >> 8) & 0xFF));
            tx.append((char)(address & 0xFF));
            tx.append((char)0x00);
            tx.append((char)0x01); // 数量=1
            tx.append((char)0x01);                       // 字节数=1
            tx.append((char)b);
        }
        else     // 05
        {
            func = 5;
            quint8 hi = value ? 0xFF : 0x00;
            tx.append((char)((address >> 8) & 0xFF));
            tx.append((char)(address & 0xFF));
            tx.append((char)hi);
            tx.append((char)0x00);
        }
        QByteArray rx;
        ok = m_client->transact(func, tx, rx, err, &txB, &rxB);
    }
    else                                  // 保持寄存器
    {
        func = (quint8)m_cfg.regWriteFunc;
        quint16 v = (quint16)value;
        if (func == 16)
        {
            tx.append((char)((address >> 8) & 0xFF));
            tx.append((char)(address & 0xFF));
            tx.append((char)0x00);
            tx.append((char)0x01); // 数量=1
            tx.append((char)0x02);                       // 字节数=2
            tx.append((char)((v >> 8) & 0xFF));
            tx.append((char)(v & 0xFF));
        }
        else     // 06
        {
            func = 6;
            tx.append((char)((address >> 8) & 0xFF));
            tx.append((char)(address & 0xFF));
            tx.append((char)((v >> 8) & 0xFF));
            tx.append((char)(v & 0xFF));
        }
        QByteArray rx;
        ok = m_client->transact(func, tx, rx, err, &txB, &rxB);
    }
    m_tx += (quint32)txB;
    if (ok) m_rx += (quint32)rxB;
    else m_errCount++;
    if (!ok)
        emit logError(QTime::currentTime().toString("hh:mm:ss.zzz"), err);
    emit writeResult(areaIndex, address, ok, ok ? QString() : err);
    emit statsUpdated(m_tx, m_rx, m_errCount);
}

void ModbusWorker::onFrameSent(const QByteArray &frame)
{
    emit logTx(m_logIsWrite, QTime::currentTime().toString("hh:mm:ss.zzz"),
               QString::fromLatin1(ModbusCodec::toHex(frame)));
}

void ModbusWorker::onFrameReceived(const QByteArray &frame)
{
    emit logRx(m_logIsWrite, QTime::currentTime().toString("hh:mm:ss.zzz"),
               QString::fromLatin1(ModbusCodec::toHex(frame)));
}
