#include "connectionpanel.h"
#include "modbuscodec.h"
#include "modbuscontroller.h"

#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSerialPortInfo>

static void setLabelWidth(QFormLayout *form, int width)
{
    for (int i = 0; i < form->rowCount(); ++i)
    {
        QLayoutItem *item = form->itemAt(i, QFormLayout::LabelRole);
        if (item && item->widget())
        {
            item->widget()->setFixedWidth(width);
        }
    }
}

ConnectionPanel::ConnectionPanel(QWidget *parent)
    : QWidget(parent)
    , m_connected(false)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(4,4,0,4);

    QGroupBox *connGrp = new QGroupBox("通道配置", this);
    QFormLayout *connLayout = new QFormLayout(connGrp);

    m_connCombo = new QComboBox(connGrp);
    m_connCombo->addItems({"串口", "网络"});
    m_connectBtn = new QPushButton("连接", connGrp);

    QHBoxLayout *connOpsLayout = new QHBoxLayout;
    connOpsLayout->addWidget(m_connCombo);
    connOpsLayout->addWidget(m_connectBtn);

    connLayout->addRow("连接方式:", connOpsLayout);
    setLabelWidth(connLayout, 55);

    m_serGroup = new QGroupBox("串口配置", this);
    QFormLayout *serLayout = new QFormLayout(m_serGroup);

    m_serPortCombo = new QComboBox(m_serGroup);
    refreshSerialPorts();

    m_baudRateCombo = new QComboBox(m_serGroup);
    m_baudRateCombo->setEditable(true);
    m_baudRateCombo->addItems({"1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"});
    m_baudRateCombo->setCurrentIndex(3);
    m_parityCombo = new QComboBox(m_serGroup);
    m_parityCombo->addItems({"无", "奇校验", "偶校验"});

    serLayout->addRow("串口号:", m_serPortCombo);
    serLayout->addRow("波特率:", m_baudRateCombo);
    serLayout->addRow("校验位:", m_parityCombo);
    setLabelWidth(serLayout, 55);

    m_netGroup = new QGroupBox("网络配置", this);
    QFormLayout *netLayout = new QFormLayout(m_netGroup);

    m_netTypeCombo = new QComboBox(m_netGroup);
    m_netTypeCombo->addItems({"TCP", "UDP"});
    m_netAddrEdit = new QLineEdit("127.0.0.1", m_netGroup);
    m_netPortSpin = new QSpinBox(m_netGroup);
    m_netPortSpin->setRange(1, 65535);
    m_netPortSpin->setValue(502);
    m_netGroup->setVisible(false);

    netLayout->addRow("网络协议:", m_netTypeCombo);
    netLayout->addRow("IP地址:", m_netAddrEdit);
    netLayout->addRow("端口号:", m_netPortSpin);
    setLabelWidth(netLayout, 55);

    QGroupBox *protoGrp = new QGroupBox("协议配置", this);
    QFormLayout *pf = new QFormLayout(protoGrp);

    m_protoCombo = new QComboBox(protoGrp);
    m_protoCombo->addItems({"Modbus-RTU", "Modbus-TCP", "Modbus-ASCII"});
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
    m_readModeCombo->addItems({"单点模式", "批量模式"});
    m_readModeCombo->setCurrentIndex(1);
    pf->addRow("读取模式:", m_readModeCombo);

    m_coilFuncCombo = new QComboBox(protoGrp);
    m_coilFuncCombo->addItems({"05", "15"});
    pf->addRow("遥控命令:", m_coilFuncCombo);

    m_regFuncCombo = new QComboBox(protoGrp);
    m_regFuncCombo->addItems({"06", "16"});
    pf->addRow("遥调命令:", m_regFuncCombo);
    setLabelWidth(pf, 55);

    root->addWidget(connGrp);
    root->addWidget(m_serGroup);
    root->addWidget(m_netGroup);
    root->addWidget(protoGrp);

    root->addWidget(new QGroupBox(this), 1);

    connect(m_connCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int idx){ m_serGroup->setVisible(idx == 0); m_netGroup->setVisible(idx == 1); });
    connect(m_connectBtn, &QPushButton::clicked, this, &ConnectionPanel::onConnectButton);

    for (QComboBox *cb : {m_protoCombo, m_readModeCombo, m_coilFuncCombo, m_regFuncCombo})
    {
        connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConnectionPanel::onProtocolChanged);
    }
    for (QSpinBox *sb : {m_slaveSpin, m_timeoutSpin, m_pollSpin})
    {
        connect(sb, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConnectionPanel::onProtocolChanged);
    }
}

void ConnectionPanel::refreshSerialPorts()
{
    QStringList ports;
    const QList<QSerialPortInfo> list = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : list)
    {
        ports.append(info.portName());
    }
    if (ports.isEmpty())
    {
        ports.append("COM1");
    }

    m_serPortCombo->clear();
    m_serPortCombo->addItems(ports);
}

const ModbusConfig& ConnectionPanel::getConfig()
{
    return m_config;
}

void ConnectionPanel::setController(IModbusController *controller)
{
    m_controller = controller;
}

void ConnectionPanel::buildConfig()
{
    m_config.transport.channel  = (ChannelType)m_connCombo->currentIndex();
    m_config.transport.portName = m_serPortCombo->currentText();
    m_config.transport.baudRate = m_baudRateCombo->currentText().toInt();
    m_config.transport.parity   = m_parityCombo->currentIndex();
    m_config.transport.netType  = (NetworkType)m_netTypeCombo->currentIndex();
    m_config.transport.netAddr  = m_netAddrEdit->text();
    m_config.transport.netPort  = m_netPortSpin->value();

    m_config.params.protocol        = (ProtocolType)m_protoCombo->currentIndex();
    m_config.params.slave           = m_slaveSpin->value();
    m_config.params.responseTimeout = m_timeoutSpin->value();
    m_config.params.pollInterval    = m_pollSpin->value();
    m_config.params.readMode        = m_readModeCombo->currentIndex();
    m_config.params.coilWriteFunc   = m_coilFuncCombo->currentText().toInt();
    m_config.params.regWriteFunc    = m_regFuncCombo->currentText().toInt();
}

void ConnectionPanel::setTransportEnabled(bool enabled)
{
    for (QWidget *w : {(QWidget *)m_connCombo, (QWidget *)m_serPortCombo, (QWidget *)m_baudRateCombo,
                       (QWidget *)m_parityCombo, (QWidget *)m_netTypeCombo, (QWidget *)m_netAddrEdit,
                       (QWidget *)m_netPortSpin})
    {
        w->setEnabled(enabled);
    }
}

void ConnectionPanel::setProtocolEnabled(bool enabled)
{
    for (QWidget *w : {(QWidget *)m_protoCombo, (QWidget *)m_slaveSpin, (QWidget *)m_timeoutSpin,
                       (QWidget *)m_pollSpin, (QWidget *)m_readModeCombo, (QWidget *)m_coilFuncCombo,
                       (QWidget *)m_regFuncCombo})
    {
        w->setEnabled(enabled);
    }
}

void ConnectionPanel::setWidgetEnabled(bool enabled)
{
    setTransportEnabled(enabled);
    setProtocolEnabled(enabled);
}

void ConnectionPanel::onConnectButton()
{
    if (!m_controller)
    {
        return;
    }
    if (m_connected)
    {
        m_controller->disconnectDevice();
        return;
    }
    m_connectBtn->setText("正在连接...");
    m_connectBtn->setEnabled(false);
    setWidgetEnabled(false);
    buildConfig();
    m_controller->connectDevice(m_config);
}

void ConnectionPanel::setConnected(bool connected)
{
    m_connected = connected;

    setTransportEnabled(!connected);
    setProtocolEnabled(true);
    m_connectBtn->setText(connected ? "断开" : "连接");
    m_connectBtn->setEnabled(true);
}

void ConnectionPanel::onProtocolChanged()
{
    if (!m_connected || !m_controller)
    {
        return;
    }
    buildConfig();
    m_controller->applyConfig(m_config.params);
}
