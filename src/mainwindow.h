#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ConnectionPanel;
class RegisterView;
class CommLogView;
class ModbusWorker;
class RealtimeData;
class QThread;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectionState(bool connected);
    void onConnectError(const QString &msg);
    void onStats(quint32 tx, quint32 rx, quint32 err);
    // 桥接：工作线程读结果 -> 中转站 API
    void onWorkerReadResult(int areaIndex, int address, int status,
                            qint64 value, const QString &errText, qint64 errValue);

private:
    ConnectionPanel *m_panel;
    RegisterView    *m_view;
    CommLogView     *m_log;   // 底部通信日志
    ModbusWorker    *m_worker;
    RealtimeData    *m_data;   // 实时数据中转站
    QThread         *m_thread;

    QLabel *m_lblConn;
    QLabel *m_lblStats;
};

#endif // MAINWINDOW_H
