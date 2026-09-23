#include "tcptransport.h"
#include "errorcodes.h"

TcpTransport::TcpTransport(const QString &ip, int port, QObject *parent)
    : ITransport(parent), m_ip(ip), m_port(port)
{
}

TcpTransport::~TcpTransport()
{
    close();
}

ErrorCode TcpTransport::open()
{
    m_tcp = new QTcpSocket(this);
    m_tcp->connectToHost(m_ip, m_port);
    if (!m_tcp->waitForConnected(3000))
    {
        return ErrorCode::TcpConnectFailed;
    }
    return ErrorCode::Ok;
}

void TcpTransport::close()
{
    delete m_tcp;
    m_tcp = nullptr;
}

bool TcpTransport::isOpen() const
{
    return m_tcp && m_tcp->state() == QAbstractSocket::ConnectedState;
}

ErrorCode TcpTransport::write(const char *data, qint64 len, qint64 *written)
{
    if (!m_tcp || m_tcp->state() != QAbstractSocket::ConnectedState)
    {
        return ErrorCode::ConnectionLost;
    }
    qint64 w = m_tcp->write(data, len);
    if (w < 0)
    {
        return ErrorCode::TcpWriteFailed;
    }
    if (written)
    {
        *written = w;
    }
    return ErrorCode::Ok;
}

ErrorCode TcpTransport::read(int timeoutMs, QByteArray &data)
{
    data.clear();
    if (!m_tcp || m_tcp->state() != QAbstractSocket::ConnectedState)
    {
        return ErrorCode::ConnectionLost;
    }
    if (!m_tcp->waitForReadyRead(timeoutMs))
    {
        return ErrorCode::TcpRecvTimeout;
    }
    data = m_tcp->readAll();
    return ErrorCode::Ok;
}
