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
    ~TcpTransport() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;
    qint64 write(const char *data, qint64 len) override;
    QByteArray read(int timeoutMs) override;
    QString errorString() const override;

private:
    QString     m_ip;
    int         m_port;
    QTcpSocket *m_tcp = nullptr;
    QString     m_err;
};

#endif
