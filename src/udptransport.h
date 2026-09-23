#ifndef UDPTRANSPORT_H
#define UDPTRANSPORT_H

#include "modbustransport.h"
#include "modbusdefs.h"
#include <QString>
#include <QObject>
#include <QUdpSocket>

class UdpTransport : public QObject, public ITransport
{
    Q_OBJECT
public:
    UdpTransport(const QString &ip, int port);
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
