#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ConnectionPanel;
class RegisterView;
class CommLogView;
class ModbusWorker;
class ModbusDevice;
class RegisterData;
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

    void onWorkerReadResult(int areaIndex, int address, int status,
                            qint64 value, const QString &errText, qint64 errValue);

    void onCountersChanged(quint32 tx, quint32 rx, quint32 errors);
    void onInfoMessage(const QString &text);
    void onErrorMessage(const QString &text);

private:
    ConnectionPanel *m_panel;
    RegisterView    *m_view;
    CommLogView     *m_log;
    ModbusDevice    *m_device;
    ModbusWorker    *m_worker;
    RegisterData    *m_data;
    QThread         *m_thread;

    QLabel *m_lblConn;
    QLabel *m_lblStats;
};

#endif
