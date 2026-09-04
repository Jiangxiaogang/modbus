#ifndef MODBUSCLIENT_H
#define MODBUSCLIENT_H

#include "modbusdefs.h"
#include "transport.h"
#include <QObject>

// 组合传输层 + 编解码，提供一次事务请求/响应
class ModbusClient : public QObject
{
    Q_OBJECT
public:
    ModbusClient(QObject* parent = 0);
    ~ModbusClient();

    bool open(const ModbusConfig& cfg);
    void close();
    bool isOpen() const;
    QString errorString() const;

    // 发送 PDU(功能码+数据)，接收响应 PDU（不含校验/MBAP）
    // 返回 false 时 err 含错误描述；txBytes/rxBytes 返回实际收发字节数
    bool transact(quint8 func, const QByteArray& txPdu,
                  QByteArray& rxPdu, QString& err,
                  qint64* txBytes = 0, qint64* rxBytes = 0);

private:
    ITransport* buildTransport(const ModbusConfig& cfg);

    ModbusConfig  m_cfg;
    ITransport*   m_transport;
    QString       m_lastErr;
};

#endif // MODBUSCLIENT_H
