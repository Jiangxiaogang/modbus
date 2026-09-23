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

    ErrorCode open() override;
    void close() override;
    bool isOpen() const override;
    ErrorCode write(const char *data, qint64 len, qint64 *written = nullptr) override;
    ErrorCode read(int timeoutMs, QByteArray &data) override;

private:
    QString     m_ip;
    int         m_port;
    QTcpSocket *m_tcp = nullptr;
};

#endif
