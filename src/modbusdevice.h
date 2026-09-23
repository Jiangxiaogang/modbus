#ifndef MODBUSDEVICE_H
#define MODBUSDEVICE_H

#include "modbuscodec.h"
#include "modbustransport.h"
#include <QObject>
#include <QVector>

class CommMonitor;

class ModbusDevice : public QObject
{
    Q_OBJECT
public:
    explicit ModbusDevice(CommMonitor *monitor, QObject *parent = nullptr);
    ~ModbusDevice() override;

    bool connectDevice(const TransportConfig &transport, const ModbusParams &params);
    void disconnectDevice();
    bool isConnected() const;
    QString errorString() const;

    void setParams(const ModbusParams &params);

    bool readBits(int readFunc, int start, int count,
                  QVector<bool> &values, QString &err, quint8 *modbusErr = nullptr);

    bool readRegisters(int readFunc, int start, int count,
                       QVector<quint16> &values, QString &err, quint8 *modbusErr = nullptr);

    bool writeCoil(int address, bool value, int coilFunc, QString &err);

    bool writeRegister(int address, quint16 value, int regFunc, QString &err);

    bool writeRegisters(int start, const QVector<quint16> &values, QString &err);

private:
    ITransport *buildTransport(const TransportConfig &transport);
    void close();

    bool transact(quint8 func, const QByteArray &txPdu,
                  QByteArray &rxPdu, QString &err,
                  qint64 *txBytes = nullptr, qint64 *rxBytes = nullptr,
                  quint8 *modbusErr = nullptr);

    bool readRequest(int readFunc, int start, int count,
                     QByteArray &rx, QString &err, quint8 *modbusErr);

    CommMonitor  *m_monitor = nullptr;
    ModbusParams  m_params;
    ITransport   *m_transport = nullptr;
    QString       m_lastErr;
};

#endif
