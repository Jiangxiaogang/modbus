#include "modbusdevice.h"
#include "modbuscodec.h"
#include "commmonitor.h"
#include "errorcodes.h"
#include "serialtransport.h"
#include "tcptransport.h"
#include "udptransport.h"

ModbusDevice::ModbusDevice(CommMonitor *monitor, QObject *parent)
    : QObject(parent), m_monitor(monitor)
{
}

ModbusDevice::~ModbusDevice()
{
    close();
}

ITransport *ModbusDevice::buildTransport(const TransportConfig &transport)
{
    if (transport.channel == ChannelSerial)
    {
        return new SerialTransport(transport.portName, transport.baudRate,
                                   transport.parity, this);
    }
    if (transport.netType == NetworkTCP)
    {
        return new TcpTransport(transport.netAddr, transport.netPort, this);
    }
    if (transport.netType == NetworkUDP)
    {
        return new UdpTransport(transport.netAddr, transport.netPort, this);
    }
    return nullptr;
}

bool ModbusDevice::connectDevice(const TransportConfig &transport, const ModbusParams &params)
{
    close();
    m_params = params;
    m_transport = buildTransport(transport);
    if (!m_transport)
    {
        m_lastErr = errorText(ErrorCode::UnsupportedChannel);
        return false;
    }
    if (!m_transport->open())
    {
        m_lastErr = m_transport->errorString();
        delete m_transport;
        m_transport = nullptr;
        return false;
    }
    return true;
}

void ModbusDevice::close()
{
    if (m_transport)
    {
        m_transport->close();
        delete m_transport;
        m_transport = nullptr;
    }
}

void ModbusDevice::disconnectDevice()
{
    close();
}

bool ModbusDevice::isConnected() const
{
    return m_transport && m_transport->isOpen();
}

QString ModbusDevice::errorString() const
{
    return m_lastErr;
}

void ModbusDevice::setParams(const ModbusParams &params)
{
    m_params = params;
}

bool ModbusDevice::transact(quint8 func, const QByteArray &txPdu,
                            QByteArray &rxPdu, QString &err,
                            qint64 *txBytes, qint64 *rxBytes,
                            quint8 *modbusErr)
{
    rxPdu.clear();
    err.clear();
    if (modbusErr)
    {
        *modbusErr = 0;
    }
    if (!isConnected())
    {
        err = errorText(ErrorCode::NotConnected);
        return false;
    }

    QByteArray frame = ModbusCodec::encode(m_params.protocol, (quint8)m_params.slave, func, txPdu);
    qint64 w = m_transport->write(frame.constData(), frame.size());
    if (w < 0)
    {
        err = m_transport->errorString();
        return false;
    }
    if (txBytes)
    {
        *txBytes = w;
    }
    if (m_monitor)
    {
        m_monitor->recordTx(frame, func);
    }

    QByteArray rx = m_transport->read(m_params.responseTimeout);
    if (rx.isEmpty())
    {
        err = errorText(ErrorCode::ResponseTimeout);
        return false;
    }
    if (rxBytes)
    {
        *rxBytes = rx.size();
    }
    if (m_monitor)
    {
        m_monitor->recordRx(rx, func);
    }

    quint8 slave = 0, rfunc = 0;
    QByteArray rPdu;
    if (!ModbusCodec::decode(m_params.protocol, rx, slave, rfunc, rPdu))
    {
        err = errorText(ErrorCode::FrameParseFailed);
        return false;
    }
    if (ModbusCodec::isException(rfunc, func))
    {
        quint8 code = rPdu.isEmpty() ? 0 : (quint8)rPdu[0];
        if (modbusErr)
        {
            *modbusErr = code;
        }
        err = errorText(ErrorCode::SlaveException, ModbusCodec::exceptionText(code));
        return false;
    }
    if (rfunc != func)
    {
        err = errorText(ErrorCode::FunctionMismatch,
                        QString("%1").arg(func, 2, 16, QLatin1Char('0')),
                        QString("%1").arg(rfunc, 2, 16, QLatin1Char('0')));
        return false;
    }
    rxPdu = rPdu;
    return true;
}

bool ModbusDevice::readRequest(int readFunc, int start, int count,
                               QByteArray &rx, QString &err, quint8 *modbusErr)
{
    QByteArray tx;
    tx.append((char)((start >> 8) & 0xFF));
    tx.append((char)(start & 0xFF));
    tx.append((char)((count >> 8) & 0xFF));
    tx.append((char)(count & 0xFF));
    bool ok = transact((quint8)readFunc, tx, rx, err, nullptr, nullptr, modbusErr);
    if (!ok)
    {
        if (m_monitor)
        {
            m_monitor->recordError(err, modbusErr ? *modbusErr : 0);
        }
    }
    return ok;
}

bool ModbusDevice::readBits(int readFunc, int start, int count,
                            QVector<bool> &values, QString &err, quint8 *modbusErr)
{
    values.clear();
    QByteArray rx;
    if (!readRequest(readFunc, start, count, rx, err, modbusErr))
    {
        return false;
    }
    if (rx.isEmpty())
    {
        err = errorText(ErrorCode::EmptyResponse);
        return false;
    }
    const char *data = rx.constData() + 1;
    values.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        values.append(((data[i / 8] >> (i % 8)) & 0x01) != 0);
    }
    return true;
}

bool ModbusDevice::readRegisters(int readFunc, int start, int count,
                                 QVector<quint16> &values, QString &err, quint8 *modbusErr)
{
    values.clear();
    QByteArray rx;
    if (!readRequest(readFunc, start, count, rx, err, modbusErr))
    {
        return false;
    }
    if (rx.isEmpty())
    {
        err = errorText(ErrorCode::EmptyResponse);
        return false;
    }
    int n = (quint8)rx[0] / 2;
    const char *data = rx.constData() + 1;
    values.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        values.append((quint16)(((quint8)data[2 * i] << 8) | (quint8)data[2 * i + 1]));
    }
    return true;
}

bool ModbusDevice::writeCoil(int address, bool value, int coilFunc, QString &err)
{
    QByteArray tx;
    tx.append((char)((address >> 8) & 0xFF));
    tx.append((char)(address & 0xFF));
    int func = coilFunc;
    if (func == 15)
    {
        tx.append((char)0x00);
        tx.append((char)0x01);
        tx.append((char)0x01);
        tx.append((char)(value ? 0x01 : 0x00));
    }
    else
    {
        func = 5;
        tx.append((char)(value ? 0xFF : 0x00));
        tx.append((char)0x00);
    }
    QByteArray rx;
    quint8 mbErr = 0;
    bool ok = transact((quint8)func, tx, rx, err, nullptr, nullptr, &mbErr);
    if (!ok)
    {
        if (m_monitor)
        {
            m_monitor->recordError(err, mbErr);
        }
    }
    return ok;
}

bool ModbusDevice::writeRegister(int address, quint16 value, int regFunc, QString &err)
{
    QByteArray tx;
    tx.append((char)((address >> 8) & 0xFF));
    tx.append((char)(address & 0xFF));
    int func = regFunc;
    if (func == 16)
    {
        tx.append((char)0x00);
        tx.append((char)0x01);
        tx.append((char)0x02);
    }
    else
    {
        func = 6;
    }
    tx.append((char)((value >> 8) & 0xFF));
    tx.append((char)(value & 0xFF));
    QByteArray rx;
    quint8 mbErr = 0;
    bool ok = transact((quint8)func, tx, rx, err, nullptr, nullptr, &mbErr);
    if (!ok)
    {
        if (m_monitor)
        {
            m_monitor->recordError(err, mbErr);
        }
    }
    return ok;
}

bool ModbusDevice::writeRegisters(int start, const QVector<quint16> &values, QString &err)
{
    QByteArray tx;
    tx.append((char)((start >> 8) & 0xFF));
    tx.append((char)(start & 0xFF));
    int qty = values.size();
    tx.append((char)((qty >> 8) & 0xFF));
    tx.append((char)(qty & 0xFF));
    tx.append((char)(qty * 2));
    for (quint16 v : values)
    {
        tx.append((char)((v >> 8) & 0xFF));
        tx.append((char)(v & 0xFF));
    }
    QByteArray rx;
    quint8 mbErr = 0;
    bool ok = transact(16, tx, rx, err, nullptr, nullptr, &mbErr);
    if (!ok)
    {
        if (m_monitor)
        {
            m_monitor->recordError(err, mbErr);
        }
    }
    return ok;
}

