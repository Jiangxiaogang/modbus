#ifndef SERIALTRANSPORT_H
#define SERIALTRANSPORT_H

#include "modbustransport.h"
#include <QString>

class QSerialPort;

class SerialTransport : public ITransport
{
public:
    SerialTransport(const QString &portName, int baudRate, int dataBits, int stopBits, int parity);
    ~SerialTransport() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;
    qint64 write(const char *data, qint64 len) override;
    QByteArray read(int timeoutMs) override;
    QString errorString() const override;

private:
    int charIntervalMs() const;

    QString      m_portName;
    int          m_baudRate;
    int          m_dataBits;
    int          m_stopBits;
    int          m_parity;
    QSerialPort *m_serial = nullptr;
    QString      m_err;
};

#endif
