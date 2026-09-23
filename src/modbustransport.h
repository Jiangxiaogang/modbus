#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <QByteArray>
#include <QString>
#include <QObject>

class ITransport : public QObject
{
public:
    explicit ITransport(QObject *parent = nullptr)
        : QObject(parent) {}
    ~ITransport() override = default;

    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual qint64 write(const char *data, qint64 len) = 0;
    virtual QByteArray read(int timeoutMs) = 0;
    virtual QString errorString() const = 0;
};

#endif
