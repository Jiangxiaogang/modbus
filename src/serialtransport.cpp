#include "serialtransport.h"

#include <QSerialPort>
#include <QThread>

static QSerialPort::DataBits toDataBits(int bits)
{
    switch (bits)
    {
    case 5: return QSerialPort::Data5;
    case 6: return QSerialPort::Data6;
    case 7: return QSerialPort::Data7;
    default: return QSerialPort::Data8;
    }
}

static QSerialPort::StopBits toStopBits(int bits)
{
    switch (bits)
    {
    case 20: return QSerialPort::TwoStop;
    case 15: return QSerialPort::OneAndHalfStop;
    default: return QSerialPort::OneStop;
    }
}

static QSerialPort::Parity toParity(int parity)
{
    switch (parity)
    {
    case 1: return QSerialPort::OddParity;  // 奇校验
    case 2: return QSerialPort::EvenParity; // 偶校验
    default: return QSerialPort::NoParity;  // 无校验
    }
}

SerialTransport::SerialTransport(const QString &portName, int baudRate,
                                 int dataBits, int stopBits, int parity)
    : m_portName(portName), m_baudRate(baudRate), m_dataBits(dataBits)
    , m_stopBits(stopBits), m_parity(parity)
{
}

SerialTransport::~SerialTransport()
{
    close();
}

int SerialTransport::charIntervalMs() const
{
    const double charBits = (m_parity == 0) ? 11.0 : 12.0; // 8N1 / 8E1 起止位
    int ms = (int)(charBits * 1000.0 / (double)m_baudRate * 3.5);
    return ms < 1 ? 1 : ms;
}

bool SerialTransport::open()
{
    m_serial = new QSerialPort;
    m_serial->setPortName(m_portName);
    m_serial->setBaudRate(m_baudRate);
    m_serial->setDataBits(toDataBits(m_dataBits));
    m_serial->setStopBits(toStopBits(m_stopBits));
    m_serial->setParity(toParity(m_parity));
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
    if (!m_serial->open(QIODevice::ReadWrite))
    {
        m_err = QString("打开串口失败: %1").arg(m_serial->errorString());
        delete m_serial;
        m_serial = nullptr;
        return false;
    }
    m_serial->clear(QSerialPort::AllDirections);
    return true;
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

qint64 SerialTransport::write(const char *data, qint64 len)
{
    if (!isOpen()) return -1;
    m_serial->clear(QSerialPort::Input); // 清掉旧数据
    qint64 w = m_serial->write(data, len);
    if (w < 0)
    {
        m_err = "串口写入失败: " + m_serial->errorString();
        return -1;
    }
    m_serial->waitForBytesWritten(1000);
    // RTU 发送后等待 3.5 字符时间，保证帧间隔
    QThread::msleep(charIntervalMs());
    return w;
}

QByteArray SerialTransport::read(int timeoutMs)
{
    QByteArray buf;
    if (!isOpen())
        return buf;
    if (!m_serial->waitForReadyRead(timeoutMs))
        return buf; // 超时无数据
    buf.append(m_serial->readAll());
    // 继续收，直到超过 3.5 字符间隔无新数据，即认为一帧结束
    const int interval = charIntervalMs();
    while (m_serial->waitForReadyRead(interval))
        buf.append(m_serial->readAll());
    return buf;
}

QString SerialTransport::errorString() const
{
    return m_err;
}
