#ifndef MODBUSWORKER_H
#define MODBUSWORKER_H

#include "modbusdefs.h"
#include <QObject>
#include <QList>
#include <QMutex>

class ModbusClient;
class QTimer;

// 在独立线程中运行：周期轮询读取 + 处理写入请求
class ModbusWorker : public QObject
{
    Q_OBJECT
public:
    explicit ModbusWorker(QObject *parent = 0);
    ~ModbusWorker();

public slots:
    void setConfig(const ModbusConfig &cfg);
    void connectDevice(const ModbusConfig &cfg);
    void disconnectDevice();
    void setAreaPlan(int areaIndex, const QList<RegPlanItem> &items);
    void writeRegister(int areaIndex, int address, DataType type, qint64 value);

signals:
    void connectionStateChanged(bool connected);
    void connectError(const QString &msg);
    void statsUpdated(quint32 tx, quint32 rx, quint32 err);
    // 单点读取结果：valid=false 且 address<0 表示整个区轮询失败
    void readResult(int areaIndex, int address, bool valid, qint64 value);
    void writeResult(int areaIndex, int address, bool ok, const QString &msg);

private slots:
    void doPoll();

private:
    void runReadArea(int areaIndex, const QList<RegPlanItem> &items);

    ModbusConfig           m_cfg;
    QList<RegPlanItem>     m_plans[4];
    ModbusClient          *m_client;
    QTimer                *m_timer;
    quint32                m_tx, m_rx, m_errCount;
    QMutex                 m_mutex;
};

#endif // MODBUSWORKER_H
