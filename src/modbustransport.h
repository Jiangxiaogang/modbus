#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <QByteArray>

class ITransport
{
public:
    virtual ~ITransport() {}
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual qint64 write(const char *data, qint64 len) = 0;
    virtual QByteArray read(int timeoutMs) = 0;
    virtual QString errorString() const = 0;
};

#endif // TRANSPORT_H
