#include "mainwindow.h"
#include "connectionpanel.h"
#include "registerview.h"
#include "commlogview.h"
#include "modbusworker.h"
#include "realtimedata.h"
#include "modbusdefs.h"
#include "aboutdialog.h"
#include "version.h"

#include <QSplitter>
#include <QStatusBar>
#include <QLabel>
#include <QThread>
#include <QList>
#include <QMessageBox>
#include <QStyleFactory>
#include <QShortcut>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QString("%1 v%2")
                   .arg(APP_PRODUCT_NAME).arg(APP_VERSION_STR));
    setMinimumSize(900, 500);
    resize(900, 560);

    // 左侧连接区 / 右侧数据区
    QSplitter *split = new QSplitter(Qt::Horizontal, this);
    m_panel = new ConnectionPanel(split);
    m_view  = new RegisterView(split);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);

    // 底部通信日志：纵向分割，拖动分割条可调整日志区高度
    m_log = new CommLogView(this);
    QSplitter *vsplit = new QSplitter(Qt::Vertical, this);
    vsplit->addWidget(split);
    vsplit->addWidget(m_log);
    vsplit->setStretchFactor(0, 1);
    vsplit->setStretchFactor(1, 0);
    vsplit->setSizes(QList<int>() << 320 << 130);
    setCentralWidget(vsplit);

    // 状态栏
    m_lblConn = new QLabel("设备未连接", this);
    m_lblStats = new QLabel("TX:0  RX:0  ERR:0", this);
    statusBar()->addWidget(m_lblConn, 1);
    statusBar()->addPermanentWidget(m_lblStats);
    statusBar()->setStyleSheet("QStatusBar {border-top: 1px solid palette(mid);}");

    // F1 弹出“关于”对话框（无菜单栏，用快捷键提供入口）
    QShortcut *aboutSc = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(aboutSc, SIGNAL(activated()), this, SLOT(showAbout()));

    // 工作线程
    m_thread = new QThread(this);
    m_worker = new ModbusWorker;
    m_worker->moveToThread(m_thread);
    connect(m_thread, SIGNAL(finished()), m_worker, SLOT(deleteLater()));
    m_thread->start();

    // 实时数据对象（中转站），位于 GUI 线程
    m_data = new RealtimeData(this);
    m_view->setRealtimeData(m_data);

    // 面板 -> 工作线程
    connect(m_panel, SIGNAL(connectClicked(ModbusConfig*)), m_worker, SLOT(connectDevice(ModbusConfig*)));
    connect(m_panel, SIGNAL(disconnectClicked()), m_worker, SLOT(disconnectDevice()));
    // 连接后协议/轮询层改动：值传递排队到工作线程热更新
    connect(m_panel, SIGNAL(configChanged(ModbusConfig)), m_worker, SLOT(applyConfig(ModbusConfig)));

    // 数据区 -> 工作线程
    connect(m_view, SIGNAL(planChanged(int, QList<RegPlanItem>*)), m_worker, SLOT(setAreaPlan(int, QList<RegPlanItem>*)));
    connect(m_view, SIGNAL(writeRequested(int, int, DataType, qint64)), m_worker, SLOT(writeRegister(int, int, DataType, qint64)));

    // 工作线程 -> UI
    connect(m_worker, SIGNAL(connectionStateChanged(bool)), this, SLOT(onConnectionState(bool)));
    connect(m_worker, SIGNAL(connectError(QString)), this, SLOT(onConnectError(QString)));
    connect(m_worker, SIGNAL(statsUpdated(quint32, quint32, quint32)), this, SLOT(onStats(quint32, quint32, quint32)));

    // 工作线程 -> 通信日志（排队连接）
    connect(m_worker, SIGNAL(logTx(bool, QString, QString)), m_log, SLOT(appendTx(bool, QString, QString)));
    connect(m_worker, SIGNAL(logRx(bool, QString, QString)), m_log, SLOT(appendRx(bool, QString, QString)));
    connect(m_worker, SIGNAL(logError(QString, QString)), m_log, SLOT(appendError(QString, QString)));
    connect(m_worker, SIGNAL(logInfo(QString, QString)), m_log, SLOT(appendInfo(QString, QString)));

    // 采集 -> 中转站 -> 展示：读到数据先经 API 写入 RealtimeData，再转发给视图显示
    connect(m_worker, SIGNAL(readResult(int, int, int, qint64, QString, qint64)),
            this,      SLOT(onWorkerReadResult(int, int, int, qint64, QString, qint64)));
    connect(m_data,   SIGNAL(dataChanged(ReadPoint)),
            m_view,    SLOT(onReadResult(ReadPoint)));

    connect(m_worker, SIGNAL(writeResult(int, int, bool, QString)), m_view, SLOT(onWriteResult(int, int, bool, QString)));
}

void MainWindow::onConnectionState(bool connected)
{
    m_panel->setConnected(connected);
    if (connected)
    {
        const ModbusConfig &cfg = m_panel->getConfig();
        QString endpoint;
        if (cfg.channel == ChannelSerial)
            endpoint = cfg.portName;
        else
            endpoint = QString("%1:%2").arg(cfg.netAddr).arg(cfg.netPort);
        m_lblConn->setText(QString("设备已连接(%1)").arg(endpoint));
        m_lblConn->setStyleSheet("QLabel{color:#0a;}");
    }
    else
    {
        m_lblConn->setText("设备未连接");
        m_lblConn->setStyleSheet("QLabel{color:#a00;}");
    }
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

void MainWindow::showAbout()
{
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::onWorkerReadResult(int areaIndex, int address, int status,
                                     qint64 value, const QString &errText, qint64 errValue)
{
    ReadPoint pt;
    pt.areaIndex = areaIndex;
    pt.address   = address;
    pt.status    = status;
    pt.value     = value;
    pt.errValue  = errValue;
    pt.errText   = errText;
    m_data->update(pt);
}
