#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <QByteArray>

// 传输层抽象：只负责收发"字节流"，不关心 Modbus 协议帧格式
class ITransport
{
public:
    virtual ~ITransport() {}
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual qint64 write(const char* data, qint64 len) = 0;
    // 读取"一帧"完整数据（传输层级别的成帧，如 RTU 的 3.5 字符间隔 / TCP 的 MBAP 长度）
    virtual QByteArray readFrame(int timeoutMs) = 0;
    virtual QString errorString() const = 0;
};

#endif // TRANSPORT_H
