#include "udptransport.h"
#include "errorcodes.h"

UdpTransport::UdpTransport(const QString &ip, int port, QObject *parent)
    : ITransport(parent), m_ip(ip), m_port(port)
{
}

UdpTransport::~UdpTransport()
{
    close();
}

ErrorCode UdpTransport::open()
{
    m_udp = new QUdpSocket(this);
    if (!m_udp->bind(0))
    {
        return ErrorCode::UdpBindFailed;
    }
    m_udp->connectToHost(m_ip, m_port);
    return ErrorCode::Ok;
}

void UdpTransport::close()
{
    delete m_udp;
    m_udp = nullptr;
}

bool UdpTransport::isOpen() const
{
    return m_udp != nullptr;
}

ErrorCode UdpTransport::write(const char *data, qint64 len, qint64 *written)
{
    if (!m_udp)
    {
        return ErrorCode::ConnectionLost;
    }
    qint64 w = m_udp->writeDatagram(data, len, m_udp->peerAddress(), m_udp->peerPort());
    if (w < 0)
    {
        return ErrorCode::UdpWriteFailed;
    }
    if (written)
    {
        *written = w;
    }
    return ErrorCode::Ok;
}

ErrorCode UdpTransport::read(int timeoutMs, QByteArray &data)
{
    data.clear();
    if (!m_udp)
    {
        return ErrorCode::ConnectionLost;
    }
    if (!m_udp->waitForReadyRead(timeoutMs))
    {
        return ErrorCode::UdpRecvTimeout;
    }
    data.resize(m_udp->pendingDatagramSize());
    m_udp->readDatagram(data.data(), data.size());
    return ErrorCode::Ok;
}
