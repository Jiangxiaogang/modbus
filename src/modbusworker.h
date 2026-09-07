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
    void connectDevice(ModbusConfig *cfg);
    void disconnectDevice();
    void setAreaPlan(int areaIndex, QList<RegPlanItem> *items);
    void writeRegister(int areaIndex, int address, DataType type, qint64 value);

signals:
    void connectionStateChanged(bool connected);
    void connectError(const QString &msg);
    void statsUpdated(quint32 tx, quint32 rx, quint32 err);
    // 单点读取结果：status 为 ReadStatus；错误时携带 Modbus 异常码与描述
    void readResult(int areaIndex, int address, int status,
                    qint64 value, const QString &errText, qint64 errValue);
    void writeResult(int areaIndex, int address, bool ok, const QString &msg);

    // 通信日志：ts 为毫秒时间戳(hh:mm:ss.zzz)，isWrite 区分读/写操作
    void logTx(bool isWrite, const QString &ts, const QString &hex);
    void logRx(bool isWrite, const QString &ts, const QString &hex);
    void logError(const QString &ts, const QString &msg);
    void logInfo(const QString &ts, const QString &msg);

private slots:
    void doPoll();
    // 转发 client 帧级收发信号，附加读/写标签与时间戳后发出
    void onFrameSent(const QByteArray &frame);
    void onFrameReceived(const QByteArray &frame);

private:
    void runReadArea(int areaIndex, const QList<RegPlanItem> &items);
    // 发起一次读请求(start 起 cnt 个)，失败仅标记分片内地址
    void readChunk(int areaIndex, int start, int cnt,
                   const QList<RegPlanItem> &items);

    ModbusConfig           m_cfg;
    QList<RegPlanItem>     m_plans[4];
    ModbusClient          *m_client;
    QTimer                *m_timer;
    quint32                m_tx, m_rx, m_errCount;
    QMutex                 m_mutex;
    bool                   m_logIsWrite; // 当前事务是否为写操作（日志标签用）
};

#endif // MODBUSWORKER_H
