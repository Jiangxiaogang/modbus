#include "mainwindow.h"
#include "connectionpanel.h"
#include "registerview.h"
#include "commlogview.h"
#include "modbusworker.h"
#include "modbusdevice.h"
#include "commmonitor.h"
#include "errorcodes.h"
#include "registerdata.h"
#include "modbuscodec.h"
#include "registertypes.h"
#include "registerpoint.h"
#include "aboutdialog.h"
#include "version.h"

#include <QSplitter>
#include <QStatusBar>
#include <QLabel>
#include <QThread>
#include <QMessageBox>
#include <QShortcut>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QString("%1 v%2").arg(APP_PRODUCT_NAME).arg(APP_VERSION_STR));
    setMinimumSize(960, 500);
    resize(960, 500);

    QSplitter *split = new QSplitter(Qt::Horizontal, this);
    m_panel = new ConnectionPanel(split);
    m_view  = new RegisterView(split);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);

    m_log = new CommLogView(this);
    QSplitter *vsplit = new QSplitter(Qt::Vertical, this);
    vsplit->addWidget(split);
    vsplit->addWidget(m_log);
    vsplit->setStretchFactor(0, 1);
    vsplit->setStretchFactor(1, 0);
    vsplit->setSizes({320, 130});
    setCentralWidget(vsplit);

    m_lblConn = new QLabel(errorText(ErrorCode::NotConnected), this);
    m_lblStats = new QLabel("TX:0  RX:0  ERR:0", this);
    statusBar()->addWidget(m_lblConn, 1);
    statusBar()->addPermanentWidget(m_lblStats);
    statusBar()->setStyleSheet("QStatusBar {border-top: 1px solid palette(mid);}");

    QShortcut *aboutSc = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(aboutSc, &QShortcut::activated, this, [this]{ AboutDialog dlg(this); dlg.exec(); });

    m_monitor = new CommMonitor(this);

    m_thread = new QThread(this);
    m_device = new ModbusDevice(m_monitor);
    m_worker = new ModbusWorker(m_device, &m_panel->getConfig());
    m_panel->setController(m_worker);
    m_view->setController(m_worker);
    m_device->moveToThread(m_thread);
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_device, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread->start();

    m_data = new RegisterData(this);
    m_view->setRegisterData(m_data);

    connect(m_worker, &ModbusWorker::connectionStateChanged, this, &MainWindow::onConnectionState);
    connect(m_worker, &ModbusWorker::connectError, this, &MainWindow::onConnectError);

    connect(m_monitor, &CommMonitor::frameSent, m_log,
            [this](const QByteArray &frame, quint8 func){ m_log->appendFrame(true, frame, func); });
    connect(m_monitor, &CommMonitor::frameReceived, m_log,
            [this](const QByteArray &frame, quint8 func){ m_log->appendFrame(false, frame, func); });
    connect(m_monitor, &CommMonitor::errorLogged, m_log,
            [this](const QString &text, quint8){ m_log->appendError(text); });
    connect(m_monitor, &CommMonitor::infoLogged, m_log, &CommLogView::appendInfo);
    connect(m_monitor, &CommMonitor::countersChanged, this, &MainWindow::onCountersChanged);

    connect(m_worker, &ModbusWorker::infoMessage,  this, &MainWindow::onInfoMessage);
    connect(m_worker, &ModbusWorker::errorMessage, this, &MainWindow::onErrorMessage);

    connect(m_worker, &ModbusWorker::readResult, this, &MainWindow::onWorkerReadResult);
    connect(m_data, &RegisterData::dataChanged, m_view, &RegisterView::onReadResult);
    connect(m_worker, &ModbusWorker::writeResult, m_view, &RegisterView::onWriteResult);
}

MainWindow::~MainWindow()
{
    m_thread->quit();
    m_thread->wait();
}

void MainWindow::onConnectionState(bool connected)
{
    m_panel->setConnected(connected);
    if (connected)
    {
        m_monitor->resetCounters();

        const ModbusConfig &cfg = m_panel->getConfig();
        QString endpoint = (cfg.transport.channel == ChannelSerial)
                           ? cfg.transport.portName
                           : QString("%1:%2").arg(cfg.transport.netAddr).arg(cfg.transport.netPort);
        m_lblConn->setText(QString("设备已连接(%1)").arg(endpoint));
        m_lblConn->setStyleSheet("QLabel{color:#0a;}");
    }
    else
    {
        m_lblConn->setText(errorText(ErrorCode::NotConnected));
        m_lblConn->setStyleSheet("QLabel{color:#a00;}");
    }
}

void MainWindow::onConnectError(const QString &msg)
{
    m_lblConn->setText("连接失败");
    m_lblConn->setStyleSheet("QLabel{color:#a00;}");
    QMessageBox::warning(this, "连接失败", msg);
}

void MainWindow::onCountersChanged(quint32 tx, quint32 rx, quint32 errors)
{
    m_lblStats->setText(QString("TX:%1  RX:%2  ERR:%3").arg(tx).arg(rx).arg(errors));
}

void MainWindow::onInfoMessage(const QString &text)
{
    m_log->appendInfo(text);
}

void MainWindow::onErrorMessage(const QString &text)
{
    m_log->appendError(text);
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
