#include "serialtransport.h"
#include "errorcodes.h"

#include <QSerialPort>
#include <QThread>

static QSerialPort::Parity toParity(int parity)
{
    switch (parity)
    {
    case 1: return QSerialPort::OddParity;
    case 2: return QSerialPort::EvenParity;
    default: return QSerialPort::NoParity;
    }
}

SerialTransport::SerialTransport(const QString &portName, int baudRate, int parity,
                                 QObject *parent)
    : ITransport(parent), m_portName(portName), m_baudRate(baudRate), m_parity(parity)
{
}

SerialTransport::~SerialTransport()
{
    close();
}

int SerialTransport::charIntervalMs() const
{
    const double charBits = (m_parity == 0) ? 11.0 : 12.0;
    int ms = (int)(charBits * 1000.0 / (double)m_baudRate * 3.5);
    return ms < 1 ? 1 : ms;
}

ErrorCode SerialTransport::open()
{
    m_serial = new QSerialPort(this);
    m_serial->setPortName(m_portName);
    m_serial->setBaudRate(m_baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setParity(toParity(m_parity));
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
    if (!m_serial->open(QIODevice::ReadWrite))
    {
        delete m_serial;
        m_serial = nullptr;
        return ErrorCode::SerialOpenFailed;
    }
    m_serial->clear(QSerialPort::AllDirections);
    return ErrorCode::Ok;
}

void SerialTransport::close()
{
    if (m_serial)
    {
        m_serial->close();
        delete m_serial;
        m_serial = nullptr;
    }
}

bool SerialTransport::isOpen() const
{
    return m_serial && m_serial->isOpen();
}

ErrorCode SerialTransport::write(const char *data, qint64 len, qint64 *written)
{
    if (!isOpen())
    {
        return ErrorCode::ConnectionLost;
    }
    m_serial->clear(QSerialPort::Input);
    qint64 w = m_serial->write(data, len);
    if (w < 0)
    {
        return ErrorCode::SerialWriteFailed;
    }
    m_serial->waitForBytesWritten(1000);

    QThread::msleep(charIntervalMs());
    if (written)
    {
        *written = w;
    }
    return ErrorCode::Ok;
}

ErrorCode SerialTransport::read(int timeoutMs, QByteArray &data)
{
    data.clear();
    if (!isOpen())
    {
        return ErrorCode::ConnectionLost;
    }
    if (!m_serial->waitForReadyRead(timeoutMs))
    {
        return ErrorCode::SerialRecvTimeout;
    }
    data.append(m_serial->readAll());

    const int interval = charIntervalMs();
    while (m_serial->waitForReadyRead(interval))
    {
        data.append(m_serial->readAll());
    }
    return ErrorCode::Ok;
}
