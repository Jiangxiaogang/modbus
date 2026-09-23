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

bool TcpTransport::open()
{
    m_tcp = new QTcpSocket(this);
    m_tcp->connectToHost(m_ip, m_port);
    if (!m_tcp->waitForConnected(3000))
    {
        m_err = errorText(ErrorCode::TcpConnectFailed, m_tcp->errorString());
        return false;
    }
    return true;
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

qint64 TcpTransport::write(const char *data, qint64 len)
{
    return m_tcp ? m_tcp->write(data, len) : -1;
}

QByteArray TcpTransport::read(int timeoutMs)
{
    if (!m_tcp->waitForReadyRead(timeoutMs))
    {
        m_err = errorText(ErrorCode::TcpRecvTimeout);
        return QByteArray();
    }
    return m_tcp->readAll();
}

QString TcpTransport::errorString() const
{
    return m_err;
}
