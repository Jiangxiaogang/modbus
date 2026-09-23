#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "errorcodes.h"
#include <QByteArray>
#include <QString>
#include <QObject>

class ITransport : public QObject
{
public:
    explicit ITransport(QObject *parent = nullptr)
        : QObject(parent) {}
    ~ITransport() override = default;

    virtual ErrorCode open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual ErrorCode write(const char *data, qint64 len, qint64 *written = nullptr) = 0;
    virtual ErrorCode read(int timeoutMs, QByteArray &data) = 0;
};

#endif
