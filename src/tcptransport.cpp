#include "tcptransport.h"

TcpTransport::TcpTransport(const QString &ip, int port)
    : m_ip(ip),
      m_port(port),
      m_tcp(0)
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
        m_err = QString("TCP 连接失败: %1").arg(m_tcp->errorString());
        return false;
    }
    return true;
}

void TcpTransport::close()
{
    if (m_tcp)
    {
        m_tcp->close();
        delete m_tcp;
        m_tcp = 0;
    }
}

bool TcpTransport::isOpen() const
{
    return m_tcp != 0 && m_tcp->state() == QAbstractSocket::ConnectedState;
}

qint64 TcpTransport::write(const char *data, qint64 len)
{
    if (!m_tcp) return -1;
    return m_tcp->write(data, len);
}

QByteArray TcpTransport::read(int timeoutMs)
{
    if(!m_tcp->waitForReadyRead(timeoutMs))
    {
        m_err = "TCP 接收超时";
        return QByteArray();
    }
    return m_tcp->readAll();
}

QString TcpTransport::errorString() const
{
    return m_err;
}
