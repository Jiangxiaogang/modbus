#ifndef MODBUSDEVICE_H
#define MODBUSDEVICE_H

#include "modbusdefs.h"
#include <QObject>
#include <QVector>

class ModbusClient;

// Modbus 设备层：在 ModbusClient(传输 + 帧编解码) 之上提供面向寄存器的读写接口，
// 负责 PDU 组包/解析与位/字数据区分。Worker 线程只调用本类，不接触底层事务细节。
class ModbusDevice : public QObject
{
    Q_OBJECT
public:
    explicit ModbusDevice(QObject *parent = nullptr);
    ~ModbusDevice() override;

    bool connectDevice(const ModbusConfig &cfg);
    void disconnectDevice();
    bool isConnected() const;
    QString errorString() const;
    void setConfig(const ModbusConfig &cfg);

    // 读位：readFunc = 1(线圈) / 2(离散输入)
    bool readBits(int readFunc, int start, int count,
                  QVector<bool> &values, QString &err, quint8 *modbusErr = nullptr);
    // 读寄存器：readFunc = 3(保持) / 4(输入)
    bool readRegisters(int readFunc, int start, int count,
                       QVector<quint16> &values, QString &err, quint8 *modbusErr = nullptr);

    // 写单个线圈：coilFunc = 5 / 15
    bool writeCoil(int address, bool value, int coilFunc, QString &err);
    // 写单个保持寄存器：regFunc = 6 / 16
    bool writeRegister(int address, quint16 value, int regFunc, QString &err);
    // 写多个保持寄存器（功能码 16），用于 32 位类型
    bool writeRegisters(int start, const QVector<quint16> &values, QString &err);

signals:
    // 完整传输帧收发通知（透传自 ModbusClient），携带请求功能码
    void frameSent(const QByteArray &frame, quint8 func);
    void frameReceived(const QByteArray &frame, quint8 func);
    // 一次事务失败（超时/异常/解析失败等），供监控器累计错误
    void operationFailed(const QString &err, quint8 modbusErr = 0);

private:
    bool readRequest(int readFunc, int start, int count,
                     QByteArray &rx, QString &err, quint8 *modbusErr);

    ModbusClient *m_client;
};

#endif // MODBUSDEVICE_H
