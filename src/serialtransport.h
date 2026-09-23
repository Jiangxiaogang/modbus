#ifndef SERIALTRANSPORT_H
#define SERIALTRANSPORT_H

#include "modbustransport.h"
#include <QString>

class QSerialPort;

class SerialTransport : public ITransport
{
public:
    SerialTransport(const QString &portName, int baudRate, int parity,
                    QObject *parent = nullptr);
    ~SerialTransport() override;

    ErrorCode open() override;
    void close() override;
    bool isOpen() const override;
    ErrorCode write(const char *data, qint64 len, qint64 *written = nullptr) override;
    ErrorCode read(int timeoutMs, QByteArray &data) override;

private:
    int charIntervalMs() const;

    QString      m_portName;
    int          m_baudRate;
    int          m_parity;
    QSerialPort *m_serial = nullptr;
};

#endif
