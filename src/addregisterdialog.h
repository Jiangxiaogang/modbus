#ifndef ADDREGISTERDIALOG_H
#define ADDREGISTERDIALOG_H

#include "modbusdefs.h"
#include <QDialog>

class QLineEdit;
class QSpinBox;
class QComboBox;
class QPushButton;

class AddRegisterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddRegisterDialog(int areaIndex, QWidget *parent = nullptr);
    int       startAddr() const;
    int       count() const;
    DataType  dataType() const;
    ByteOrder byteOrder() const;

private:

    void refreshByteOrderOptions();

    QLineEdit   *m_addrEdit;
    QSpinBox    *m_countSpin;
    QComboBox   *m_typeCombo;
    QComboBox   *m_byteOrderCombo;
    QPushButton *m_okBtn;
};

#endif
