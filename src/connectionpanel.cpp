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
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        char name[256];
        char val[256];
        DWORD nameLen, valLen, type;
        for (DWORD i = 0; ; ++i)
        {
            nameLen = sizeof(name);
            valLen = sizeof(val);
            LONG r = RegEnumValueA(hKey, i, name, &nameLen, 0, &type, (LPBYTE)val, &valLen);
            if (r != ERROR_SUCCESS) break;
            ports.append(QString::fromLocal8Bit(val, valLen));
        }
        RegCloseKey(hKey);
    }
    if (ports.isEmpty())
    {
        for (int i = 1; i <= 20; ++i)
            ports.append(QString("COM%1").arg(i));
    }
    return ports;
}

ConnectionPanel::ConnectionPanel(QWidget *parent)
    : QWidget(parent)
    , m_connected(false)
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setMargin(4);

    // ---------- 串口通道配置 ----------
    QGroupBox *serChanGrp = new QGroupBox("串口通道", this);
    QFormLayout *serChanLayout = new QFormLayout(serChanGrp);

    m_serPortCombo = new QComboBox(serChanGrp);
    refreshSerialPorts();
    m_baudRateCombo = new QComboBox(serChanGrp);
    m_baudRateCombo->setEditable(true);
    QStringList bauds;
    bauds << "1200" << "2400" << "4800" << "9600" << "19200" << "38400" << "57600" << "115200";
    m_baudRateCombo->addItems(bauds);
    m_baudRateCombo->setCurrentIndex(3);
    m_dataBitsCombo = new QComboBox(serChanGrp);
    m_dataBitsCombo->addItems(QStringList() << "7" << "8");
    m_dataBitsCombo->setCurrentIndex(3);
    m_stopBitsCombo = new QComboBox(serChanGrp);
    m_stopBitsCombo->addItems(QStringList() << "1" << "2");
    m_stopBitsCombo->setCurrentIndex(0);
    m_parityCombo = new QComboBox(serChanGrp);
    m_parityCombo->addItems(QStringList() << "无(N)" << "奇校验(O)" << "偶校验(E)");
    m_parityCombo->setCurrentIndex(0);
    m_serConnectBtn = new QPushButton("连接(&S)", serChanGrp);

    serChanLayout->addRow("串口号:", m_serPortCombo);
    serChanLayout->addRow("波特率:", m_baudRateCombo);
    serChanLayout->addRow("数据位:", m_dataBitsCombo);
    serChanLayout->addRow("停止位:", m_stopBitsCombo);
    serChanLayout->addRow("校验位:", m_parityCombo);
    serChanLayout->addRow(m_serConnectBtn);

    // ---------- 网络通道配置 ----------
    QGroupBox *netChanGrp = new QGroupBox("网络通道", this);
    QFormLayout *netChanLayout = new QFormLayout(netChanGrp);

    m_netTypeCombo = new QComboBox(netChanGrp);
    m_netTypeCombo->addItem("TCP");
    m_netTypeCombo->addItem("UDP");

    m_netAddrEdit = new QLineEdit("127.0.0.1", netChanGrp);
    m_netPortSpin = new QSpinBox(netChanGrp);
    m_netPortSpin->setRange(1, 65535);
    m_netPortSpin->setValue(502);
    m_netConnectBtn = new QPushButton("连接(&N)", netChanGrp);

    netChanLayout->addRow("网络协议:", m_netTypeCombo);
    netChanLayout->addRow("IP地址:", m_netAddrEdit);
    netChanLayout->addRow("端口号:", m_netPortSpin);
    netChanLayout->addRow(m_netConnectBtn);

    // ---------- 协议配置 ----------
    QGroupBox *protoGrp = new QGroupBox("协议配置", this);
    QFormLayout *pf = new QFormLayout(protoGrp);

    m_protoCombo = new QComboBox(protoGrp);
    m_protoCombo->addItem("ModbusRTU");
    m_protoCombo->addItem("ModbusTCP");
    m_protoCombo->addItem("ModbusASCII");
    pf->addRow("协议类型:", m_protoCombo);

    m_slaveSpin = new QSpinBox(protoGrp);
    m_slaveSpin->setRange(1, 255);
    m_slaveSpin->setValue(1);
    pf->addRow("从站地址:", m_slaveSpin);

    m_timeoutSpin = new QSpinBox(protoGrp);
    m_timeoutSpin->setRange(100, 10000);
    m_timeoutSpin->setValue(1000);
    m_timeoutSpin->setSuffix(" ms");
    pf->addRow("响应超时:", m_timeoutSpin);

    m_pollSpin = new QSpinBox(protoGrp);
    m_pollSpin->setRange(100, 10000);
    m_pollSpin->setValue(1000);
    m_pollSpin->setSuffix(" ms");
    pf->addRow("轮询间隔:", m_pollSpin);

    m_quantSpin = new QSpinBox(protoGrp);
    m_quantSpin->setRange(1, 127);
    m_quantSpin->setValue(127);
    pf->addRow("单次读取数量:", m_quantSpin);

    m_coilFuncCombo = new QComboBox(protoGrp);
    m_coilFuncCombo->addItem("05");
    m_coilFuncCombo->addItem("15");
    pf->addRow("遥控功能码:", m_coilFuncCombo);

    m_regFuncCombo = new QComboBox(protoGrp);
    m_regFuncCombo->addItem("06");
    m_regFuncCombo->addItem("16");
    pf->addRow("遥调功能码:", m_regFuncCombo);

    root->addWidget(serChanGrp);
    root->addWidget(netChanGrp);
    root->addWidget(protoGrp);
    root->addStretch(1);

    // 信号连接
    connect(m_serConnectBtn, SIGNAL(clicked()), this, SLOT(onSerConnectButton()));
    connect(m_netConnectBtn, SIGNAL(clicked()), this, SLOT(onNetConnectButton()));
}

void ConnectionPanel::refreshSerialPorts()
{
    m_serPortCombo->clear();
    m_serPortCombo->addItems(getSerialPorts());
}

const ModbusConfig& ConnectionPanel::getConfig()
{
    return m_config;
}

void ConnectionPanel::buildConfig(ChannelType chan)
{
    m_config.channel         = chan;
    m_config.protocol        = (ProtocolType)m_protoCombo->currentIndex();
    m_config.serPortName     = m_serPortCombo->currentText();
    m_config.baudRate        = m_baudRateCombo->currentText().toInt();
    m_config.dataBits        = m_dataBitsCombo->currentText().toInt();
    m_config.stopBits        = m_stopBitsCombo->currentText().toInt();
    m_config.parity          = m_parityCombo->currentIndex();

    m_config.netAddr         = m_netAddrEdit->text();
    m_config.netPort         = m_netPortSpin->value();

    m_config.slave           = m_slaveSpin->value();
    m_config.responseTimeout = m_timeoutSpin->value();
    m_config.pollInterval    = m_pollSpin->value();
    m_config.readQuantity    = m_quantSpin->value();
    m_config.coilWriteFunc   = m_coilFuncCombo->currentText().toInt();
    m_config.regWriteFunc    = m_regFuncCombo->currentText().toInt();
}

void ConnectionPanel::setWidgetEnabled(bool enabled)
{
    m_serPortCombo->setEnabled(enabled);
    m_baudRateCombo->setEnabled(enabled);
    m_dataBitsCombo->setEnabled(enabled);
    m_stopBitsCombo->setEnabled(enabled);
    m_parityCombo->setEnabled(enabled);
    m_serConnectBtn->setEnabled(enabled);

    m_netTypeCombo->setEnabled(enabled);
    m_netAddrEdit->setEnabled(enabled);
    m_netPortSpin->setEnabled(enabled);
    m_netConnectBtn->setEnabled(enabled);

    m_protoCombo->setEnabled(enabled);
    m_slaveSpin->setEnabled(enabled);
    m_timeoutSpin->setEnabled(enabled);
    m_pollSpin->setEnabled(enabled);
    m_quantSpin->setEnabled(enabled);
    m_coilFuncCombo->setEnabled(enabled);
    m_regFuncCombo->setEnabled(enabled);
}

void ConnectionPanel::onSerConnectButton()
{
    if (m_connected)
    {
        emit disconnectClicked();
    }
    else
    {
        setWidgetEnabled(false);
        buildConfig(ChannelSerial);
        emit connectClicked();
    }
}

void ConnectionPanel::onNetConnectButton()
{
    if (m_connected)
    {
        emit disconnectClicked();
    }
    else
    {
        int type = m_netTypeCombo->currentIndex();
        setWidgetEnabled(false);
        buildConfig(type ? ChannelTcp : ChannelUdp);
        emit connectClicked();
    }
}

void ConnectionPanel::setConnected(bool connected)
{
    m_connected = connected;
    if(!connected)
    {
        setWidgetEnabled(true);
    }
    if(m_config.channel == ChannelSerial)
    {
        m_serConnectBtn->setText(connected ? "断开" : "连接");
        m_serConnectBtn->setEnabled(true);
    }
    else
    {
        m_netConnectBtn->setText(connected ? "断开" : "连接");
        m_netConnectBtn->setEnabled(true);
    }
}
