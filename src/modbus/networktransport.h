#ifndef NETWORKTRANSPORT_H
#define NETWORKTRANSPORT_H

#include "transport.h"
#include <QString>
#include <QObject>

class QTcpSocket;
class QUdpSocket;

// 网络传输：TCP 或 UDP（均承载 ModbusTCP/MBAP 帧）
class NetworkTransport : public QObject, public ITransport
{
    Q_OBJECT
public:
    NetworkTransport(const QString& ip, int port, bool udp);
    ~NetworkTransport();

    bool open();
    void close();
    bool isOpen() const;
    qint64 write(const char* data, qint64 len);
    QByteArray readFrame(int timeoutMs);
    QString errorString() const;

private:
    QByteArray readExact(qint64 n, int timeoutMs);

    QString    m_ip;
    int        m_port;
    bool       m_udp;
    QTcpSocket* m_tcp;
    QUdpSocket* m_udpSock;
    QString    m_err;
};

#endif // NETWORKTRANSPORT_H
