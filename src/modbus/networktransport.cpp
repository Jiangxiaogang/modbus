#include "networktransport.h"
#include <QTcpSocket>
#include <QUdpSocket>

NetworkTransport::NetworkTransport(const QString& ip, int port, bool udp)
    : m_ip(ip), m_port(port), m_udp(udp), m_tcp(0), m_udpSock(0)
{
}

NetworkTransport::~NetworkTransport()
{
    close();
}

bool NetworkTransport::open()
{
    if (m_udp) {
        m_udpSock = new QUdpSocket(this);
        // 绑定随机本地端口，并设定对端，使 write/readDatagram 可用
        if (!m_udpSock->bind(0)) {
            m_err = "UDP 绑定失败";
            return false;
        }
        m_udpSock->connectToHost(m_ip, m_port);
        return true;
    } else {
        m_tcp = new QTcpSocket(this);
        m_tcp->connectToHost(m_ip, m_port);
        if (!m_tcp->waitForConnected(3000)) {
            m_err = QString("TCP 连接失败: %1").arg(m_tcp->errorString());
            return false;
        }
        return true;
    }
}

void NetworkTransport::close()
{
    if (m_tcp) { m_tcp->close(); delete m_tcp; m_tcp = 0; }
    if (m_udpSock) { m_udpSock->close(); delete m_udpSock; m_udpSock = 0; }
}

bool NetworkTransport::isOpen() const
{
    if (m_udp)
        return m_udpSock != 0;
    return m_tcp != 0 && m_tcp->state() == QAbstractSocket::ConnectedState;
}

qint64 NetworkTransport::write(const char* data, qint64 len)
{
    if (m_udp) {
        if (!m_udpSock) return -1;
        return m_udpSock->writeDatagram(data, len, m_udpSock->peerAddress(), m_udpSock->peerPort());
    }
    if (!m_tcp) return -1;
    return m_tcp->write(data, len);
}

QByteArray NetworkTransport::readExact(qint64 n, int timeoutMs)
{
    QByteArray buf;
    buf.reserve((int)n);
    int elapsed = 0;
    while (buf.size() < n) {
        if (m_udp) {
            if (!m_udpSock->waitForReadyRead(200)) {
                elapsed += 200;
                if (elapsed >= timeoutMs) break;
                continue;
            }
            while (m_udpSock->hasPendingDatagrams()) {
                QByteArray dgram;
                dgram.resize(m_udpSock->pendingDatagramSize());
                m_udpSock->readDatagram(dgram.data(), dgram.size());
                buf.append(dgram);
            }
        } else {
            if (!m_tcp->waitForReadyRead(200)) {
                elapsed += 200;
                if (elapsed >= timeoutMs) break;
                continue;
            }
            buf.append(m_tcp->readAll());
        }
        if (buf.size() >= n) break;
    }
    return buf;
}

QByteArray NetworkTransport::readFrame(int timeoutMs)
{
    if (m_udp) {
        if (!m_udpSock->waitForReadyRead(timeoutMs)) {
            m_err = "UDP 接收超时";
            return QByteArray();
        }
        QByteArray dgram;
        dgram.resize(m_udpSock->pendingDatagramSize());
        m_udpSock->readDatagram(dgram.data(), dgram.size());
        return dgram;
    }
    // TCP：先读 MBAP 头(6字节) 得到长度，再读剩余
    QByteArray header = readExact(6, timeoutMs);
    if (header.size() < 6) {
        m_err = "TCP 接收超时/头不完整";
        return QByteArray();
    }
    int len = ((quint8)header[4] << 8) | (quint8)header[5];
    QByteArray body = readExact(len, timeoutMs);
    if (body.size() < len) {
        m_err = "TCP 报文不完整";
        return QByteArray();
    }
    return header + body;
}

QString NetworkTransport::errorString() const
{
    return m_err;
}
