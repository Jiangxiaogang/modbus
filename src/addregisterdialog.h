#ifndef ADDREGISTERDIALOG_H
#define ADDREGISTERDIALOG_H

#include "modbusdefs.h"
#include <QDialog>

class QLineEdit;
class QSpinBox;
class QComboBox;
class QPushButton;

// 添加点位：输入起始地址、数量、数据类型与字节序
// 位区(DO/DI) 固定为 bit 且无字节序，对应控件禁用
class AddRegisterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddRegisterDialog(int areaIndex, QWidget *parent = nullptr);
    int       startAddr() const;  // 起始地址 (0-65535)
    int       count() const;      // 数量
    DataType  dataType() const;   // 数据类型
    ByteOrder byteOrder() const;  // 字节序

private:
    // 按当前数据类型刷新可选字节序（16位 AB/BA，32位 ABCD/CDAB/BADC/DCBA）
    void refreshByteOrderOptions();

    QLineEdit   *m_addrEdit;
    QSpinBox    *m_countSpin;
    QComboBox   *m_typeCombo;
    QComboBox   *m_byteOrderCombo;
    QPushButton *m_okBtn;
};

#endif // ADDREGISTERDIALOG_H
