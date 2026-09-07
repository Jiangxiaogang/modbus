#include "modbusclient.h"
#include "modbuscodec.h"
#include "serialtransport.h"
#include "tcptransport.h"
#include "udptransport.h"

ModbusClient::ModbusClient(QObject *parent)
    : QObject(parent), m_transport(0)
{
}

ModbusClient::~ModbusClient()
{
    close();
}

ITransport *ModbusClient::buildTransport(const ModbusConfig &cfg)
{
    if (cfg.channel == ChannelSerial)
        return new SerialTransport(cfg.portName, cfg.baudRate, cfg.dataBits, cfg.stopBits, cfg.parity);
    if (cfg.netType == NetworkTCP)
        return new TcpTransport(cfg.netAddr, cfg.netPort);
    if (cfg.netType == NetworkUDP)
        return new UdpTransport(cfg.netAddr, cfg.netPort);
    return NULL;
}

bool ModbusClient::open(const ModbusConfig &cfg)
{
    close();
    m_cfg = cfg;
    m_transport = buildTransport(cfg);
    if (!m_transport->open())
    {
        m_lastErr = m_transport->errorString();
        delete m_transport;
        m_transport = 0;
        return false;
    }
    return true;
}

void ModbusClient::close()
{
    if (m_transport)
    {
        m_transport->close();
        delete m_transport;
        m_transport = 0;
    }
}

bool ModbusClient::isOpen() const
{
    return m_transport && m_transport->isOpen();
}

QString ModbusClient::errorString() const
{
    return m_lastErr;
}

bool ModbusClient::transact(quint8 func, const QByteArray &txPdu,
                            QByteArray &rxPdu, QString &err,
                            qint64 *txBytes, qint64 *rxBytes,
                            quint8 *modbusErr)
{
    rxPdu.clear();
    err.clear();
    if (modbusErr) *modbusErr = 0;
    if (!isOpen())
    {
        err = "未连接";
        return false;
    }

    QByteArray frame = ModbusCodec::encode(m_cfg.protocol, (quint8)m_cfg.slave,
                                           func, txPdu);
    qint64 w = m_transport->write(frame.constData(), frame.size());
    if (w < 0)
    {
        err = m_transport->errorString();
        return false;
    }
    if (txBytes) *txBytes = w;
    emit frameSent(frame);

    QByteArray rx = m_transport->read(m_cfg.responseTimeout);
    if (rx.isEmpty())
    {
        err = "响应超时";
        return false;
    }
    if (rxBytes) *rxBytes = rx.size();
    emit frameReceived(rx);

    quint8 slave = 0, rfunc = 0;
    QByteArray rPdu;
    if (!ModbusCodec::decode(m_cfg.protocol, rx, slave, rfunc, rPdu))
    {
        err = "响应解析失败(校验/格式错误)";
        return false;
    }
    if (ModbusCodec::isException(rfunc, func))
    {
        quint8 code = rPdu.isEmpty() ? 0 : (quint8)rPdu[0];
        if (modbusErr) *modbusErr = code;
        err = "从站异常: " + ModbusCodec::exceptionText(code);
        return false;
    }
    if (rfunc != func)
    {
        err = QString("功能码不匹配 请求0x%1 响应0x%2")
              .arg(func, 2, 16, QLatin1Char('0'))
              .arg(rfunc, 2, 16, QLatin1Char('0'));
        return false;
    }
    rxPdu = rPdu;
    return true;
}
