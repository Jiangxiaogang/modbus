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

    // 丢弃上一轮超时之后才到达的陈旧应答，避免它被误当成本次请求的响应
    //（事务号固定，无法靠 MBAP 事务号区分）。注意 flush() 只清发送缓冲，
    // 不能清接收缓冲，这里必须把接收侧读空：
    // 先清掉已进入 Qt 缓冲区的内容，再用 0 超时探测一次内核接收缓冲区。
    m_tcp->readAll();
    if (m_tcp->waitForReadyRead(0))
    {
        m_tcp->readAll();
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
