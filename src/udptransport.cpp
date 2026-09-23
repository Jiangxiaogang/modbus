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

bool UdpTransport::open()
{
    m_udp = new QUdpSocket(this);
    if (!m_udp->bind(0))
    {
        m_err = errorText(ErrorCode::UdpBindFailed);
        return false;
    }
    m_udp->connectToHost(m_ip, m_port);
    return true;
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

qint64 UdpTransport::write(const char *data, qint64 len)
{
    if (!m_udp)
    {
        return -1;
    }
    return m_udp->writeDatagram(data, len, m_udp->peerAddress(), m_udp->peerPort());
}

QByteArray UdpTransport::read(int timeoutMs)
{
    if (!m_udp->waitForReadyRead(timeoutMs))
    {
        m_err = errorText(ErrorCode::UdpRecvTimeout);
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
