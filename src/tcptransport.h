#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "modbustransport.h"
#include <QString>
#include <QTcpSocket>

class TcpTransport : public ITransport
{
public:
    TcpTransport(const QString &ip, int port, QObject *parent = nullptr);
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
