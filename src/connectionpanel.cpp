#include "connectionpanel.h"
#include "modbusdefs.h"
#include "seriallist.h"

#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QIntValidator>

ConnectionPanel::ConnectionPanel(QWidget *parent)
    : QWidget(parent)
    , m_connected(false)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setMargin(0);
    setContentsMargins(4,4,0,4);
    // ---------- 通道配置 ----------
    QGroupBox *connGrp = new QGroupBox("通道配置", this);
    QFormLayout *connLayout = new QFormLayout(connGrp);

    m_connCombo = new QComboBox(connGrp);
    m_connCombo->addItem("串口");
    m_connCombo->addItem("网络");
    m_connCombo->setCurrentIndex(0);

    m_connectBtn = new QPushButton("连接", connGrp);

    QHBoxLayout *connOpsLayout = new QHBoxLayout;
    connOpsLayout->addWidget(m_connCombo);
    connOpsLayout->addWidget(m_connectBtn);
    connOpsLayout->setStretchFactor(m_connCombo, 1);
    connOpsLayout->setStretchFactor(m_connectBtn, 1);

    connLayout->addRow("连接方式:", connOpsLayout);
    for(int i = 0; i < connLayout->rowCount(); i++)
    {
        QLayoutItem *item = connLayout->itemAt(i, QFormLayout::LabelRole);
        item->widget()->setFixedWidth(55);
    }

    // ---------- 串口通道配置 ----------
    m_serGroup = new QGroupBox("串口配置", this);
    QFormLayout *serLayout = new QFormLayout(m_serGroup);

    m_serPortCombo = new QComboBox(m_serGroup);
    refreshSerialPorts();

    m_baudRateCombo = new QComboBox(m_serGroup);
    m_baudRateCombo->setEditable(true);
    QStringList bauds;
    bauds << "1200" << "2400" << "4800" << "9600" << "19200" << "38400" << "57600" << "115200";
    m_baudRateCombo->addItems(bauds);
    m_baudRateCombo->setCurrentIndex(3);
    m_parityCombo = new QComboBox(m_serGroup);
    m_parityCombo->addItems(QStringList() << "无" << "奇校验" << "偶校验");
    m_parityCombo->setCurrentIndex(0);

    serLayout->addRow("串口号:", m_serPortCombo);
    serLayout->addRow("波特率:", m_baudRateCombo);
    serLayout->addRow("校验位:", m_parityCombo);
    for(int i = 0; i < serLayout->rowCount(); i++)
    {
        QLayoutItem *item = serLayout->itemAt(i, QFormLayout::LabelRole);
        item->widget()->setFixedWidth(55);
    }

    // ---------- 网络通道配置 ----------
    m_netGroup = new QGroupBox("网络配置", this);
    QFormLayout *netLayout = new QFormLayout(m_netGroup);

    m_netTypeCombo = new QComboBox(m_netGroup);
    m_netTypeCombo->addItem("TCP");
    m_netTypeCombo->addItem("UDP");
    m_netTypeCombo->setCurrentIndex(0);
    m_netAddrEdit = new QLineEdit("127.0.0.1", m_netGroup);
    m_netPortSpin = new QSpinBox(m_netGroup);
    m_netPortSpin->setRange(1, 65535);
    m_netPortSpin->setValue(502);
    m_netGroup->setVisible(false);

    netLayout->addRow("网络协议:", m_netTypeCombo);
    netLayout->addRow("IP地址:", m_netAddrEdit);
    netLayout->addRow("端口号:", m_netPortSpin);
    for(int i = 0; i < netLayout->rowCount(); i++)
    {
        QLayoutItem *item = netLayout->itemAt(i, QFormLayout::LabelRole);
        item->widget()->setFixedWidth(55);
    }

    // ---------- 协议配置 ----------
    QGroupBox *protoGrp = new QGroupBox("协议配置", this);
    QFormLayout *pf = new QFormLayout(protoGrp);

    m_protoCombo = new QComboBox(protoGrp);
    m_protoCombo->addItem("Modbus-RTU");
    m_protoCombo->addItem("Modbus-TCP");
    m_protoCombo->addItem("Modbus-ASCII");
    pf->addRow("协议类型:", m_protoCombo);

    m_slaveSpin = new QSpinBox(protoGrp);
    m_slaveSpin->setRange(1, 255);
    m_slaveSpin->setValue(1);
    pf->addRow("从站地址:", m_slaveSpin);

    m_timeoutSpin = new QSpinBox(protoGrp);
    m_timeoutSpin->setRange(100, 10000);
    m_timeoutSpin->setValue(1000);
    m_timeoutSpin->setSuffix(" 毫秒");
    pf->addRow("响应超时:", m_timeoutSpin);

    m_pollSpin = new QSpinBox(protoGrp);
    m_pollSpin->setRange(100, 10000);
    m_pollSpin->setValue(1000);
    m_pollSpin->setSuffix(" 毫秒");
    pf->addRow("轮询间隔:", m_pollSpin);

    m_readModeCombo = new QComboBox(protoGrp);
    m_readModeCombo->addItem("单点模式");
    m_readModeCombo->addItem("批量模式");
    m_readModeCombo->setCurrentIndex(1);
    pf->addRow("读取模式:", m_readModeCombo);

    m_coilFuncCombo = new QComboBox(protoGrp);
    m_coilFuncCombo->addItem("05");
    m_coilFuncCombo->addItem("15");
    pf->addRow("遥控命令:", m_coilFuncCombo);

    m_regFuncCombo = new QComboBox(protoGrp);
    m_regFuncCombo->addItem("06");
    m_regFuncCombo->addItem("16");
    pf->addRow("遥调命令:", m_regFuncCombo);

    for(int i = 0; i < pf->rowCount(); i++)
    {
        QLayoutItem *item = pf->itemAt(i, QFormLayout::LabelRole);
        item->widget()->setFixedWidth(55);
    }

    root->addWidget(connGrp);
    root->addWidget(m_serGroup);
    root->addWidget(m_netGroup);
    root->addWidget(protoGrp);
    // 下方可伸缩占位组框：填充与底部通信日志之间的空白区域
    root->addWidget(new QGroupBox(this), 1);

    // 信号连接
    connect(m_connCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onConnChanged(int)));
    connect(m_connectBtn, SIGNAL(clicked()), this, SLOT(onConnectButton()));
}

void ConnectionPanel::refreshSerialPorts()
{
    m_serPortCombo->clear();
    m_serPortCombo->addItems(SerialList::getSerialPorts());
}

const ModbusConfig& ConnectionPanel::getConfig()
{
    return m_config;
}

void ConnectionPanel::buildConfig()
{
    m_config.channel         = (ChannelType)m_connCombo->currentIndex();
    m_config.protocol        = (ProtocolType)m_protoCombo->currentIndex();
    m_config.portName        = m_serPortCombo->currentText();
    m_config.baudRate        = m_baudRateCombo->currentText().toInt();
    m_config.dataBits        = 8;
    m_config.stopBits        = 1;
    m_config.parity          = m_parityCombo->currentIndex();
    m_config.netType         = (NetworkType)m_netTypeCombo->currentIndex();
    m_config.netAddr         = m_netAddrEdit->text();
    m_config.netPort         = m_netPortSpin->value();

    m_config.slave           = m_slaveSpin->value();
    m_config.responseTimeout = m_timeoutSpin->value();
    m_config.pollInterval    = m_pollSpin->value();
    m_config.readMode        = m_readModeCombo->currentIndex();
    m_config.coilWriteFunc   = m_coilFuncCombo->currentText().toInt();
    m_config.regWriteFunc    = m_regFuncCombo->currentText().toInt();
}

void ConnectionPanel::setWidgetEnabled(bool enabled)
{
    m_connCombo->setEnabled(enabled);
    m_serPortCombo->setEnabled(enabled);
    m_baudRateCombo->setEnabled(enabled);
    m_parityCombo->setEnabled(enabled);

    m_netTypeCombo->setEnabled(enabled);
    m_netAddrEdit->setEnabled(enabled);
    m_netPortSpin->setEnabled(enabled);

    m_protoCombo->setEnabled(enabled);
    m_slaveSpin->setEnabled(enabled);
    m_timeoutSpin->setEnabled(enabled);
    m_pollSpin->setEnabled(enabled);
    m_readModeCombo->setEnabled(enabled);
    m_coilFuncCombo->setEnabled(enabled);
    m_regFuncCombo->setEnabled(enabled);
}

void ConnectionPanel::onConnChanged(int idx)
{
    m_serGroup->setVisible(idx == 0);
    m_netGroup->setVisible(idx == 1);
}

void ConnectionPanel::onConnectButton()
{
    if (m_connected)
    {
        emit disconnectClicked();
    }
    else
    {
        m_connectBtn->setText("正在连接...");
        m_connectBtn->setEnabled(false);
        setWidgetEnabled(false);
        buildConfig();
        emit connectClicked(&m_config);
    }
}

void ConnectionPanel::setConnected(bool connected)
{
    m_connected = connected;
    setWidgetEnabled(!connected);
    m_connectBtn->setText(connected ? "断开" : "连接");
    m_connectBtn->setEnabled(true);
}
