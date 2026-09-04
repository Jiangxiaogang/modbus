#ifndef ADDREGISTERDIALOG_H
#define ADDREGISTERDIALOG_H

#include <QDialog>

// 快速添加寄存器：输入起始 PLC 地址与数量
class AddRegisterDialog : public QDialog
{
    Q_OBJECT
public:
    AddRegisterDialog(int areaIndex, QWidget *parent = 0);
    int startPlc() const;   // 起始 PLC 地址 (如 40001)
    int count() const;      // 数量

private slots:
    void validate();

private:
    int m_areaIndex;
    class QLineEdit   *m_addrEdit;
    class QSpinBox    *m_countSpin;
    class QPushButton *m_okBtn;
};

#endif // ADDREGISTERDIALOG_H
