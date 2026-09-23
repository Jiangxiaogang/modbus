#ifndef MODBUSCLIENT_H
#define MODBUSCLIENT_H

#include "modbusdefs.h"
#include "modbustransport.h"
#include <QObject>

// 组合传输层 + 编解码，提供一次事务请求/响应
class ModbusClient : public QObject
{
    Q_OBJECT
public:
    explicit ModbusClient(QObject *parent = nullptr);
    ~ModbusClient() override;

    bool open(const ModbusConfig &cfg);
    // 热更新协议层参数(协议/从站/超时)，不重建传输层
    void setConfig(const ModbusConfig &cfg);
    void close();
    bool isOpen() const;
    QString errorString() const;

    // 发送 PDU(功能码+数据)，接收响应 PDU（不含校验/MBAP）
    // 返回 false 时 err 含错误描述；txBytes/rxBytes 返回实际收发字节数
    // modbusErr 返回异常响应中的 Modbus 异常码(0x01..)，无异常时为 0
    bool transact(quint8 func, const QByteArray &txPdu,
                  QByteArray &rxPdu, QString &err,
                  qint64 *txBytes = nullptr, qint64 *rxBytes = nullptr,
                  quint8 *modbusErr = nullptr);

signals:
    // 完整传输帧（含从站/校验/MBAP）收发通知，携带请求功能码供监控分类
    void frameSent(const QByteArray &frame, quint8 func);
    void frameReceived(const QByteArray &frame, quint8 func);

private:
    ITransport *buildTransport(const ModbusConfig &cfg);

    ModbusConfig  m_cfg;
    ITransport   *m_transport = nullptr;
    QString       m_lastErr;
};

#endif // MODBUSCLIENT_H
