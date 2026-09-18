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
    void connectClicked(ModbusConfig *config);
    void disconnectClicked();
    void configChanged(const ModbusConfig &config);

public slots:
    void setConnected(bool connected);

private slots:
    void onConnChanged(int idx);
    void onConnectButton();
    void onProtocolChanged();

private:
    void refreshSerialPorts();
    void buildConfig();
    void setWidgetEnabled(bool enabled);
    void setTransportEnabled(bool enabled);
    void setProtocolEnabled(bool enabled);
    bool         m_connected;
    ModbusConfig m_config;

    QComboBox   *m_connCombo;
    QPushButton *m_connectBtn;

    QGroupBox   *m_serGroup;
    QComboBox   *m_serPortCombo;
    QComboBox   *m_baudRateCombo;
    QComboBox   *m_parityCombo;

    QGroupBox   *m_netGroup;
    QComboBox   *m_netTypeCombo;
    QLineEdit   *m_netAddrEdit;
    QSpinBox    *m_netPortSpin;

    QComboBox   *m_protoCombo;
    QSpinBox    *m_slaveSpin;
    QSpinBox    *m_timeoutSpin;
    QSpinBox    *m_pollSpin;
    QComboBox   *m_readModeCombo;
    QComboBox   *m_coilFuncCombo;
    QComboBox   *m_regFuncCombo;
};

#endif // CONNECTIONPANEL_H
