#include "udptransport.h"
#include "modbuscodec.h"
#include <QUdpSocket>

UdpTransport::UdpTransport(const QString &ip, int port)
    : m_ip(ip)
    , m_port(port)
    , m_udp(0)
{
}

UdpTransport::~UdpTransport()
{
    close();
}

bool UdpTransport::open()
{
    m_udp = new QUdpSocket(this);
    if (!m_udp->bind(0))
    {
        m_err = "UDP 绑定失败";
        return false;
    }
    m_udp->connectToHost(m_ip, m_port);
    return true;
}

void UdpTransport::close()
{
    if (m_udp)
    {
        m_udp->close();
        delete m_udp;
        m_udp = 0;
    }
}

bool UdpTransport::isOpen() const
{
    return m_udp != 0;
}

qint64 UdpTransport::write(const char *data, qint64 len)
{
    if (!m_udp) return -1;
    return m_udp->writeDatagram(data, len, m_udp->peerAddress(), m_udp->peerPort());
}

QByteArray UdpTransport::read(int timeoutMs)
{
    if (!m_udp->waitForReadyRead(timeoutMs))
    {
        m_err = "UDP 接收超时";
        return QByteArray();
    }
    QByteArray dgram;
    dgram.resize(m_udp->pendingDatagramSize());
    m_udp->readDatagram(dgram.data(), dgram.size());
    return dgram;
}

QString UdpTransport::errorString() const
{
    return m_err;
}
