#include "mainwindow.h"
#include "connectionpanel.h"
#include "registerview.h"
#include "commlogview.h"
#include "modbusworker.h"
#include "modbusdevice.h"
#include "registerdata.h"
#include "modbusdefs.h"
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

    m_lblConn = new QLabel("设备未连接", this);
    m_lblStats = new QLabel("TX:0  RX:0  ERR:0", this);
    statusBar()->addWidget(m_lblConn, 1);
    statusBar()->addPermanentWidget(m_lblStats);
    statusBar()->setStyleSheet("QStatusBar {border-top: 1px solid palette(mid);}");

    QShortcut *aboutSc = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(aboutSc, &QShortcut::activated, this, [this]{ AboutDialog dlg(this); dlg.exec(); });

    m_thread = new QThread(this);
    m_device = new ModbusDevice;
    m_worker = new ModbusWorker(m_device, &m_panel->getConfig());
    m_device->moveToThread(m_thread);
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_device, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread->start();

    m_data = new RegisterData(this);
    m_view->setRegisterData(m_data);

    connect(m_panel, &ConnectionPanel::connectClicked, m_worker, &ModbusWorker::connectDevice);
    connect(m_panel, &ConnectionPanel::disconnectClicked, m_worker, &ModbusWorker::disconnectDevice);
    connect(m_panel, &ConnectionPanel::configChanged, m_worker, &ModbusWorker::applyConfig);

    connect(m_view, &RegisterView::planChanged, m_worker, &ModbusWorker::setAreaPlan);
    connect(m_view, &RegisterView::writeRequested, m_worker, &ModbusWorker::writeRegister);

    connect(m_worker, &ModbusWorker::connectionStateChanged, this, &MainWindow::onConnectionState);
    connect(m_worker, &ModbusWorker::connectError, this, &MainWindow::onConnectError);

    connect(m_device, &ModbusDevice::frameSent,       this, &MainWindow::onFrameSent);
    connect(m_device, &ModbusDevice::frameReceived,   this, &MainWindow::onFrameReceived);
    connect(m_device, &ModbusDevice::operationFailed, this, &MainWindow::onOperationError);

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

        m_tx = m_rx = m_err = 0;
        updateStats();

        const ModbusConfig &cfg = m_panel->getConfig();
        QString endpoint = (cfg.channel == ChannelSerial)
                           ? cfg.portName
                           : QString("%1:%2").arg(cfg.netAddr).arg(cfg.netPort);
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

void MainWindow::updateStats()
{
    m_lblStats->setText(QString("TX:%1  RX:%2  ERR:%3").arg(m_tx).arg(m_rx).arg(m_err));
}

void MainWindow::onFrameSent(const QByteArray &frame, quint8 func)
{
    ++m_tx;
    m_log->appendFrame(true, frame, func);
    updateStats();
}

void MainWindow::onFrameReceived(const QByteArray &frame, quint8 func)
{
    ++m_rx;
    m_log->appendFrame(false, frame, func);
    updateStats();
}

void MainWindow::onOperationError(const QString &err, quint8 modbusErr)
{
    Q_UNUSED(modbusErr);
    ++m_err;
    m_log->appendError(err);
    updateStats();
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
