#ifndef SERIALTRANSPORT_H
#define SERIALTRANSPORT_H

#include "modbustransport.h"
#include <QString>

// 基于 Win32 API 的串口传输（Qt4.8 无内置 QSerialPort）
class SerialTransport : public ITransport
{
public:
    SerialTransport(const QString &portName, int baudRate, int dataBits, int stopBits, int parity);
    ~SerialTransport();

    bool open();
    void close();
    bool isOpen() const;
    qint64 write(const char *data, qint64 len);
    QByteArray read(int timeoutMs);
    QString errorString() const;

private:
    QString m_portName;
    int     m_baudRate;
    int     m_dataBits;
    int     m_stopBits;
    int     m_parity;
    void   *m_handle;   // HANDLE
    QString m_err;
};

#endif // SERIALTRANSPORT_H
