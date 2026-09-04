#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "modbustransport.h"
#include "modbusdefs.h"
#include <QString>
#include <QObject>
#include <QTcpSocket>

class TcpTransport : public QObject, public ITransport
{
    Q_OBJECT
public:
    TcpTransport(const QString &ip, int port);
    ~TcpTransport();

    bool open();
    void close();
    bool isOpen() const;
    qint64 write(const char *data, qint64 len);
    QByteArray read(int timeoutMs);
    QString errorString() const;

private:
    QString     m_ip;
    int         m_port;
    QTcpSocket *m_tcp;
    QString     m_err;
};

#endif // TCPTRANSPORT_H
