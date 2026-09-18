#ifndef ADDREGISTERDIALOG_H
#define ADDREGISTERDIALOG_H

#include <QDialog>

// 快速添加寄存器：输入起始地址与数量
class AddRegisterDialog : public QDialog
{
    Q_OBJECT
public:
    AddRegisterDialog(int areaIndex, QWidget *parent = 0);
    int startAddr() const;  // 起始地址 (0-65535)
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
