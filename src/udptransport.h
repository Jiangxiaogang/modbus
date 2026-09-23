#ifndef UDPTRANSPORT_H
#define UDPTRANSPORT_H

#include "modbustransport.h"
#include <QString>
#include <QUdpSocket>

class UdpTransport : public ITransport
{
public:
    UdpTransport(const QString &ip, int port, QObject *parent = nullptr);
    ~UdpTransport() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;
    qint64 write(const char *data, qint64 len) override;
    QByteArray read(int timeoutMs) override;
    QString errorString() const override;

private:
    QString     m_ip;
    int         m_port;
    QUdpSocket *m_udp = nullptr;
    QString     m_err;
};

#endif
