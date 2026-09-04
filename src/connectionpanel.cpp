#include "connectionpanel.h"
#include "modbusdefs.h"

#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QIntValidator>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

static QStringList getSerialPorts()
{
    QStringList ports;
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                     "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0,
                     KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        char name[256];
        char val[256];
        DWORD nameLen, valLen, type;
        for (DWORD i = 0; ; ++i) {
            nameLen = sizeof(name);
            valLen = sizeof(val);
            LONG r = RegEnumValueA(hKey, i, name, &nameLen, 0, &type,
                                   (LPBYTE)val, &valLen);
            if (r != ERROR_SUCCESS) break;
            ports.append(QString::fromLocal8Bit(val, valLen));
        }
        RegCloseKey(hKey);
    }
    if (ports.isEmpty()) {
        for (int i = 1; i <= 20; ++i)
            ports.append(QString("COM%1").arg(i));
    }
    return ports;
}

ConnectionPanel::ConnectionPanel(QWidget* parent)
    : QWidget(parent), m_connected(false)
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setMargin(4);

    // ---------- 通道配置 ----------
    QGroupBox* chanGrp = new QGroupBox("通道配置", this);
    QFormLayout* chan = new QFormLayout(chanGrp);

    m_channelCombo = new QComboBox(chanGrp);
    m_channelCombo->addItem("串口");
    m_channelCombo->addItem("TCP");
    m_channelCombo->addItem("UDP");
    chan->addRow("通道类型", m_channelCombo);

    m_serialBox = new QGroupBox("串口参数", chanGrp);
    QFormLayout* sf = new QFormLayout(m_serialBox);
    m_portCombo = new QComboBox(m_serialBox);
    refreshSerialPorts();
    m_baudCombo = new QComboBox(m_serialBox);
    m_baudCombo->setEditable(true);
    QStringList bauds; bauds << "1200" << "2400" << "4800" << "9600"
                             << "19200" << "38400" << "57600" << "115200";
    m_baudCombo->addItems(bauds);
    m_baudCombo->setCurrentIndex(3);
    m_dataBitsCombo = new QComboBox(m_serialBox);
    m_dataBitsCombo->addItems(QStringList() << "5" << "6" << "7" << "8");
    m_dataBitsCombo->setCurrentIndex(3);
    m_stopBitsCombo = new QComboBox(m_serialBox);
    m_stopBitsCombo->addItems(QStringList() << "1" << "1.5" << "2");
    m_stopBitsCombo->setCurrentIndex(0);
    m_parityCombo = new QComboBox(m_serialBox);
    m_parityCombo->addItems(QStringList() << "无(N)" << "奇(O)" << "偶(E)");
    m_parityCombo->setCurrentIndex(0);
    sf->addRow("串口号", m_portCombo);
    sf->addRow("波特率", m_baudCombo);
    sf->addRow("数据位", m_dataBitsCombo);
    sf->addRow("停止位", m_stopBitsCombo);
    sf->addRow("校验位", m_parityCombo);
    chan->addRow(m_serialBox);

    m_netBox = new QGroupBox("网络参数", chanGrp);
    QFormLayout* nf = new QFormLayout(m_netBox);
    m_ipEdit = new QLineEdit("127.0.0.1", m_netBox);
    m_portSpin = new QSpinBox(m_netBox);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(502);
    nf->addRow("IP地址", m_ipEdit);
    nf->addRow("端口号", m_portSpin);
    chan->addRow(m_netBox);

    m_connectBtn = new QPushButton("连接", chanGrp);
    chan->addRow(m_connectBtn);

    // ---------- 协议配置 ----------
    QGroupBox* protoGrp = new QGroupBox("协议配置", this);
    QFormLayout* pf = new QFormLayout(protoGrp);

    m_protoCombo = new QComboBox(protoGrp);
    m_protoCombo->addItem("ModbusRTU");
    m_protoCombo->addItem("ModbusASCII");
    pf->addRow("协议类型", m_protoCombo);

    m_slaveSpin = new QSpinBox(protoGrp);
    m_slaveSpin->setRange(1, 247);
    m_slaveSpin->setValue(1);
    pf->addRow("从站地址", m_slaveSpin);

    m_timeoutSpin = new QSpinBox(protoGrp);
    m_timeoutSpin->setRange(100, 10000);
    m_timeoutSpin->setValue(1000);
    m_timeoutSpin->setSuffix(" ms");
    pf->addRow("响应超时", m_timeoutSpin);

    m_pollSpin = new QSpinBox(protoGrp);
    m_pollSpin->setRange(100, 10000);
    m_pollSpin->setValue(1000);
    m_pollSpin->setSuffix(" ms");
    pf->addRow("轮询间隔", m_pollSpin);

    m_quantSpin = new QSpinBox(protoGrp);
    m_quantSpin->setRange(1, 2000);
    m_quantSpin->setValue(127);
    pf->addRow("单次读取数量", m_quantSpin);

    m_coilFuncCombo = new QComboBox(protoGrp);
    m_coilFuncCombo->addItem("05");
    m_coilFuncCombo->addItem("15");
    pf->addRow("遥控功能码", m_coilFuncCombo);

    m_regFuncCombo = new QComboBox(protoGrp);
    m_regFuncCombo->addItem("06");
    m_regFuncCombo->addItem("16");
    pf->addRow("遥调功能码", m_regFuncCombo);

    root->addWidget(chanGrp);
    root->addWidget(protoGrp);
    root->addStretch(1);

    // 信号连接
    connect(m_channelCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onChannelChanged(int)));
    connect(m_protoCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onProtocolChanged(int)));
    connect(m_connectBtn, SIGNAL(clicked()),
            this, SLOT(onConnectButton()));

    // 任意配置变化
    connect(m_portCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onAnyConfigChanged()));
    connect(m_baudCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onAnyConfigChanged()));
    connect(m_baudCombo, SIGNAL(editTextChanged(QString)), this, SLOT(onAnyConfigChanged()));
    connect(m_dataBitsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onAnyConfigChanged()));
    connect(m_stopBitsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onAnyConfigChanged()));
    connect(m_parityCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onAnyConfigChanged()));
    connect(m_ipEdit, SIGNAL(textChanged(QString)), this, SLOT(onAnyConfigChanged()));
    connect(m_portSpin, SIGNAL(valueChanged(int)), this, SLOT(onAnyConfigChanged()));
    connect(m_slaveSpin, SIGNAL(valueChanged(int)), this, SLOT(onAnyConfigChanged()));
    connect(m_timeoutSpin, SIGNAL(valueChanged(int)), this, SLOT(onAnyConfigChanged()));
    connect(m_pollSpin, SIGNAL(valueChanged(int)), this, SLOT(onAnyConfigChanged()));
    connect(m_quantSpin, SIGNAL(valueChanged(int)), this, SLOT(onAnyConfigChanged()));
    connect(m_coilFuncCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onAnyConfigChanged()));
    connect(m_regFuncCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onAnyConfigChanged()));

    onChannelChanged(0);
}

void ConnectionPanel::refreshSerialPorts()
{
    m_portCombo->clear();
    m_portCombo->addItems(getSerialPorts());
}

void ConnectionPanel::onChannelChanged(int idx)
{
    bool serial = (idx == 0);
    m_serialBox->setVisible(serial);
    m_netBox->setVisible(!serial);
    // 协议：串口支持 RTU/ASCII；网络仅 ModbusTCP
    if (serial) {
        m_protoCombo->clear();
        m_protoCombo->addItem("ModbusRTU");
        m_protoCombo->addItem("ModbusASCII");
        m_protoCombo->setCurrentIndex(0);
    } else {
        m_protoCombo->clear();
        m_protoCombo->addItem("ModbusTCP");
        m_protoCombo->setCurrentIndex(0);
    }
    onAnyConfigChanged();
}

void ConnectionPanel::onProtocolChanged(int /*idx*/)
{
    onAnyConfigChanged();
}

ModbusConfig ConnectionPanel::buildConfig() const
{
    ModbusConfig cfg;
    int ch = m_channelCombo->currentIndex();
    cfg.channel = (ch == 0) ? ChannelSerial : (ch == 1 ? ChannelTcp : ChannelUdp);

    int proto = m_protoCombo->currentIndex();
    if (cfg.channel == ChannelSerial)
        cfg.protocol = (proto == 0) ? ProtocolRTU : ProtocolASCII;
    else
        cfg.protocol = ProtocolTCP;

    cfg.portName  = m_portCombo->currentText();
    cfg.baudRate  = m_baudCombo->currentText().toInt();
    cfg.dataBits  = m_dataBitsCombo->currentText().toInt();
    cfg.stopBits  = (int)(m_stopBitsCombo->currentText().toDouble() * 10);
    cfg.parity    = m_parityCombo->currentText().contains("奇") ? 'O'
                  : m_parityCombo->currentText().contains("偶") ? 'E' : 'N';

    cfg.ipAddress = m_ipEdit->text();
    cfg.port      = m_portSpin->value();

    cfg.slave           = m_slaveSpin->value();
    cfg.responseTimeout = m_timeoutSpin->value();
    cfg.pollInterval    = m_pollSpin->value();
    cfg.readQuantity    = m_quantSpin->value();
    cfg.coilWriteFunc   = m_coilFuncCombo->currentText().toInt();
    cfg.regWriteFunc    = m_regFuncCombo->currentText().toInt();
    return cfg;
}

void ConnectionPanel::onAnyConfigChanged()
{
    emit configChanged(buildConfig());
}

void ConnectionPanel::onConnectButton()
{
    if (m_connected)
        emit disconnectClicked();
    else
        emit connectClicked();
}

void ConnectionPanel::setConnected(bool connected)
{
    m_connected = connected;
    m_connectBtn->setText(connected ? "断开" : "连接");
}

void ConnectionPanel::emitConfigChanged()
{
    emit configChanged(buildConfig());
}
