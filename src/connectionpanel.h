#ifndef CONNECTIONPANEL_H
#define CONNECTIONPANEL_H

#include "modbusdefs.h"
#include <QWidget>
#include <QStringList>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QGroupBox>

//通道配置 + 协议配置
class ConnectionPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ConnectionPanel(QWidget *parent = 0);
    const ModbusConfig& getConfig();

signals:
    void connectClicked();
    void disconnectClicked();

public slots:
    void setConnected(bool connected);

private slots:
    void onSerConnectButton();
    void onNetConnectButton();

private:
    void refreshSerialPorts();
    void buildConfig(ChannelType chan);
    void setWidgetEnabled(bool enabled);
    bool         m_connected;
    ModbusConfig m_config;

    QComboBox   *m_serPortCombo;
    QComboBox   *m_baudRateCombo;
    QComboBox   *m_dataBitsCombo;
    QComboBox   *m_stopBitsCombo;
    QComboBox   *m_parityCombo;
    QPushButton *m_serConnectBtn;

    QComboBox   *m_netTypeCombo;
    QLineEdit   *m_netAddrEdit;
    QSpinBox    *m_netPortSpin;
    QPushButton *m_netConnectBtn;

    QComboBox   *m_protoCombo;
    QSpinBox    *m_slaveSpin;
    QSpinBox    *m_timeoutSpin;
    QSpinBox    *m_pollSpin;
    QSpinBox    *m_quantSpin;
    QComboBox   *m_coilFuncCombo;
    QComboBox   *m_regFuncCombo;
};

#endif // CONNECTIONPANEL_H
