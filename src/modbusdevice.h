#ifndef MODBUSDEVICE_H
#define MODBUSDEVICE_H

#include "modbusdefs.h"
#include "modbustransport.h"
#include <QObject>
#include <QVector>

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

    bool readBits(int readFunc, int start, int count,
                  QVector<bool> &values, QString &err, quint8 *modbusErr = nullptr);

    bool readRegisters(int readFunc, int start, int count,
                       QVector<quint16> &values, QString &err, quint8 *modbusErr = nullptr);

    bool writeCoil(int address, bool value, int coilFunc, QString &err);

    bool writeRegister(int address, quint16 value, int regFunc, QString &err);

    bool writeRegisters(int start, const QVector<quint16> &values, QString &err);

signals:

    void frameSent(const QByteArray &frame, quint8 func);
    void frameReceived(const QByteArray &frame, quint8 func);

    void operationFailed(const QString &err, quint8 modbusErr = 0);

private:
    ITransport *buildTransport(const ModbusConfig &cfg);
    void close();

    bool transact(quint8 func, const QByteArray &txPdu,
                  QByteArray &rxPdu, QString &err,
                  qint64 *txBytes = nullptr, qint64 *rxBytes = nullptr,
                  quint8 *modbusErr = nullptr);

    bool readRequest(int readFunc, int start, int count,
                     QByteArray &rx, QString &err, quint8 *modbusErr);

    ModbusConfig  m_cfg;
    ITransport   *m_transport = nullptr;
    QString       m_lastErr;
};

#endif
