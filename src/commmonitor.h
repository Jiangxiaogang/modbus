#ifndef COMMMONITOR_H
#define COMMMONITOR_H

#include "commevent.h"
#include <QObject>

// 报文监控器：唯一负责把底层帧/错误加工为 CommEvent、维护通信统计(TX/RX/ERR)。
// 运行于工作线程，直接接收设备信号；对 UI 以队列信号方式输出。
class CommMonitor : public QObject
{
    Q_OBJECT
public:
    explicit CommMonitor(QObject *parent = nullptr);

public slots:
    void onFrameSent(const QByteArray &frame, quint8 func);
    void onFrameReceived(const QByteArray &frame, quint8 func);
    void onOperationError(const QString &err, quint8 modbusErr);
    void reportInfo(const QString &text);
    void reportError(const QString &text);
    void resetStats();

signals:
    void eventAppended(const CommEvent &ev);
    void statsUpdated(quint32 tx, quint32 rx, quint32 err);

private:
    void append(const CommEvent &ev);
    void emitStats();
    static bool funcIsWrite(quint8 func);
    static QString frameText(const QByteArray &frame);

    quint32 m_tx = 0;
    quint32 m_rx = 0;
    quint32 m_errCount = 0;
};

#endif // COMMMONITOR_H
