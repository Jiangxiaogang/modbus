#ifndef ADDREGISTERDIALOG_H
#define ADDREGISTERDIALOG_H

#include <QDialog>

class QLineEdit;
class QSpinBox;
class QPushButton;

// 快速添加寄存器：输入起始地址与数量
class AddRegisterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddRegisterDialog(int areaIndex, QWidget *parent = nullptr);
    int startAddr() const;  // 起始地址 (0-65535)
    int count() const;      // 数量

private:
    QLineEdit   *m_addrEdit;
    QSpinBox    *m_countSpin;
    QPushButton *m_okBtn;
};

#endif // ADDREGISTERDIALOG_H
