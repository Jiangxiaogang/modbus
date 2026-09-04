#include "mainwindow.h"
#include "connectionpanel.h"
#include "registerview.h"
#include "modbusworker.h"
#include "modbusdefs.h"

#include <QSplitter>
#include <QStatusBar>
#include <QLabel>
#include <QThread>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("ModbusTool");
    setMinimumSize(900, 520);
    resize(900, 520);

    // 左侧连接区 / 右侧数据区
    QSplitter *split = new QSplitter(Qt::Horizontal, this);
    m_panel = new ConnectionPanel(split);
    m_view  = new RegisterView(split);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    setCentralWidget(split);

    // 状态栏
    m_lblConn = new QLabel("未连接", this);
    m_lblStats = new QLabel("TX:0  RX:0  ERR:0", this);
    statusBar()->addWidget(m_lblConn, 0);
    statusBar()->addWidget(m_lblStats, 1);

    // 工作线程
    m_thread = new QThread(this);
    m_worker = new ModbusWorker;
    m_worker->moveToThread(m_thread);
    connect(m_thread, SIGNAL(finished()), m_worker, SLOT(deleteLater()));
    m_thread->start();

    // 面板 -> 工作线程
    connect(m_panel, SIGNAL(connectClicked()), m_worker, SLOT(connectDevice()));
    connect(m_panel, SIGNAL(disconnectClicked()), m_worker, SLOT(disconnectDevice()));

    // 数据区 -> 工作线程
    connect(m_view, SIGNAL(planChanged(int, QList<RegPlanItem>)), m_worker, SLOT(setAreaPlan(int, QList<RegPlanItem>)));
    connect(m_view, SIGNAL(writeRequested(int, int, DataType, qint64)), m_worker, SLOT(writeRegister(int, int, DataType, qint64)));

    // 工作线程 -> UI
    connect(m_worker, SIGNAL(connectionStateChanged(bool)), this, SLOT(onConnectionState(bool)));
    connect(m_worker, SIGNAL(connectError(QString)), this, SLOT(onConnectError(QString)));
    connect(m_worker, SIGNAL(statsUpdated(quint32, quint32, quint32)), this, SLOT(onStats(quint32, quint32, quint32)));
    connect(m_worker, SIGNAL(readResult(int, int, bool, qint64)), m_view, SLOT(onReadResult(int, int, bool, qint64)));
    connect(m_worker, SIGNAL(writeResult(int, int, bool, QString)), m_view, SLOT(onWriteResult(int, int, bool, QString)));
}

void MainWindow::onConnectionState(bool connected)
{
    m_panel->setConnected(connected);
    m_lblConn->setText(connected ? "已连接" : "未连接");
    m_lblConn->setStyleSheet(connected? "QLabel{color:#0a0;}": "QLabel{color:#a00;}");
}

void MainWindow::onConnectError(const QString &msg)
{
    m_lblConn->setText("连接失败");
    m_lblConn->setStyleSheet("QLabel{color:#a00;}");
    QMessageBox::warning(this, "连接失败", msg);
}

void MainWindow::onStats(quint32 tx, quint32 rx, quint32 err)
{
    m_lblStats->setText(QString("TX:%1  RX:%2  ERR:%3")
                        .arg(tx).arg(rx).arg(err));
}
