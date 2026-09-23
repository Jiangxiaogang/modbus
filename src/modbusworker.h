#ifndef MODBUSWORKER_H
#define MODBUSWORKER_H

#include "modbusdefs.h"
#include <QObject>
#include <QList>
#include <QMutex>

class ModbusDevice;
class CommMonitor;
class QTimer;

// 在独立线程中运行：周期轮询读取 + 处理写入请求；读写经 ModbusDevice，
// 报文监控交给 CommMonitor，本类不再关心格式化/计数/日志。
class ModbusWorker : public QObject
{
    Q_OBJECT
public:
    explicit ModbusWorker(QObject *parent = nullptr);
    ~ModbusWorker() override;

    // 报文监控器（位于本对象同一线程），供 UI 侧连接其信号
    CommMonitor *monitor() const { return m_monitor; }

public slots:
    void setConfig(const ModbusConfig &cfg);
    void applyConfig(const ModbusConfig &cfg);
    void connectDevice(ModbusConfig *cfg);
    void disconnectDevice();
    void setAreaPlan(int areaIndex, QList<RegPlanItem> *items);
    void writeRegister(int areaIndex, int address, DataType type,
                       ByteOrder byteOrder, qint64 value);

signals:
    void connectionStateChanged(bool connected);
    void connectError(const QString &msg);
    // 单点读取结果：status 为 ReadStatus；错误时携带 Modbus 异常码与描述
    void readResult(int areaIndex, int address, int status,
                    qint64 value, const QString &errText, qint64 errValue);
    void writeResult(int areaIndex, int address, bool ok, const QString &msg);

private slots:
    void doPoll();

private:
    void runReadArea(int areaIndex, const QList<RegPlanItem> &items);
    // 发起一次读请求(start 起 cnt 个)，失败仅标记分片内地址
    void readChunk(int areaIndex, int start, int cnt, const QList<RegPlanItem> &items);

    ModbusConfig       m_cfg;
    QList<RegPlanItem> m_plans[4];
    ModbusDevice      *m_device  = nullptr;
    CommMonitor       *m_monitor = nullptr;
    QTimer            *m_timer   = nullptr;
    QMutex             m_mutex;
};

#endif // MODBUSWORKER_H
