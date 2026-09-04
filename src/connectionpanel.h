#ifndef CONNECTIONPANEL_H
#define CONNECTIONPANEL_H

#include "modbusdefs.h"
#include <QWidget>
#include <QStringList>

class QComboBox;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QGroupBox;

// 左侧连接会话区：通道配置 + 协议配置
class ConnectionPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ConnectionPanel(QWidget* parent = 0);

signals:
    void configChanged(const ModbusConfig& cfg);
    void connectClicked();
    void disconnectClicked();

public slots:
    void setConnected(bool connected);
    void emitConfigChanged();

private slots:
    void onChannelChanged(int idx);
    void onProtocolChanged(int idx);
    void onAnyConfigChanged();
    void onConnectButton();

private:
    ModbusConfig buildConfig() const;
    void refreshSerialPorts();

    QComboBox* m_channelCombo;
    QGroupBox* m_serialBox;
    QGroupBox* m_netBox;
    QComboBox* m_portCombo;
    QComboBox* m_baudCombo;
    QComboBox* m_dataBitsCombo;
    QComboBox* m_stopBitsCombo;
    QComboBox* m_parityCombo;
    QLineEdit* m_ipEdit;
    QSpinBox*  m_portSpin;

    QComboBox* m_protoCombo;
    QSpinBox*  m_slaveSpin;
    QSpinBox*  m_timeoutSpin;
    QSpinBox*  m_pollSpin;
    QSpinBox*  m_quantSpin;
    QComboBox* m_coilFuncCombo;
    QComboBox* m_regFuncCombo;

    QPushButton* m_connectBtn;
    bool m_connected;
};

#endif // CONNECTIONPANEL_H
