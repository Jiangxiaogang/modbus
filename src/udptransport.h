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
    ~UdpTransport();

    bool open();
    void close();
    bool isOpen() const;
    qint64 write(const char *data, qint64 len);
    QByteArray read(int timeoutMs);
    QString errorString() const;

private:
    QString     m_ip;
    int         m_port;
    QUdpSocket *m_udp;
    QString     m_err;
};

#endif // UDPTRANSPORT_H
