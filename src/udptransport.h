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

    ErrorCode open() override;
    void close() override;
    bool isOpen() const override;
    ErrorCode write(const char *data, qint64 len, qint64 *written = nullptr) override;
    ErrorCode read(int timeoutMs, QByteArray &data) override;

private:
    QString     m_ip;
    int         m_port;
    QUdpSocket *m_udp = nullptr;
};

#endif
