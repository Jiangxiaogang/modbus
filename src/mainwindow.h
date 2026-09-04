#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ConnectionPanel;
class RegisterView;
class ModbusWorker;
class QThread;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = 0);

private slots:
    void onConnectionState(bool connected);
    void onConnectError(const QString& msg);
    void onStats(quint32 tx, quint32 rx, quint32 err);

private:
    ConnectionPanel* m_panel;
    RegisterView*    m_view;
    ModbusWorker*    m_worker;
    QThread*         m_thread;

    QLabel* m_lblConn;
    QLabel* m_lblStats;
};

#endif // MAINWINDOW_H
