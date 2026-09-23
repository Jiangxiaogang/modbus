#ifndef MODBUSWORKER_H
#define MODBUSWORKER_H

#include "modbusdefs.h"
#include <QObject>
#include <QList>
#include <QMutex>

class ModbusDevice;
class QTimer;

class ModbusWorker : public QObject
{
    Q_OBJECT
public:
    ModbusWorker(ModbusDevice *device, const ModbusConfig *cfg, QObject *parent = nullptr);
    ~ModbusWorker() override;

    void applyConfig();
    void connectDevice();
    void disconnectDevice();
    void setAreaPlan(int areaIndex, const QList<RegPlanItem> &items);
    void writeRegister(int areaIndex, int address, DataType type,
                       ByteOrder byteOrder, qint64 value);

signals:
    void connectionStateChanged(bool connected);
    void connectError(const QString &msg);
    void readResult(int areaIndex, int address, int status,
                    qint64 value, const QString &errText, qint64 errValue);
    void writeResult(int areaIndex, int address, bool ok, const QString &msg);
    void infoMessage(const QString &text);
    void errorMessage(const QString &text);

private slots:
    void doPoll();

private:
    void doApplyConfig();
    void doConnectDevice();
    void doDisconnectDevice();
    void doSetAreaPlan(int areaIndex, const QList<RegPlanItem> &items);
    void doWriteRegister(int areaIndex, int address, DataType type,
                         ByteOrder byteOrder, qint64 value);

    void runReadArea(int areaIndex, const QList<RegPlanItem> &items);
    void readChunk(int areaIndex, int start, int cnt, const QList<RegPlanItem> &items);

    const ModbusConfig *m_cfgPtr = nullptr;
    ModbusConfig        m_cfg;
    QList<RegPlanItem>  m_plans[4];
    ModbusDevice       *m_device = nullptr;
    QTimer             *m_timer  = nullptr;
    QMutex              m_mutex;
};

#endif
