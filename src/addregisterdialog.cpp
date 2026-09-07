#include "addregisterdialog.h"
#include "modbusdefs.h"
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QDialogButtonBox>

AddRegisterDialog::AddRegisterDialog(int areaIndex, QWidget *parent)
    : QDialog(parent), m_areaIndex(areaIndex)
{
    setWindowTitle(QString("快速添加"));
    setModal(true);

    m_addrSpin = new QSpinBox(this);
    m_addrSpin->setRange(0, 65535);
    m_addrSpin->setValue(0);

    m_countSpin = new QSpinBox(this);
    m_countSpin->setRange(1, 65535);
    m_countSpin->setValue(10);

    QFormLayout *form = new QFormLayout;
    form->addRow("地址:", m_addrSpin);
    form->addRow("数量:", m_countSpin);

    m_okBtn = new QPushButton("确定", this);
    QPushButton *cancel = new QPushButton("取消", this);
    QDialogButtonBox *bbox = new QDialogButtonBox;
    bbox->addButton(m_okBtn, QDialogButtonBox::AcceptRole);
    bbox->addButton(cancel, QDialogButtonBox::RejectRole);
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_addrSpin, SIGNAL(valueChanged(int)), this, SLOT(validate()));

    QVBoxLayout *v = new QVBoxLayout(this);
    v->addLayout(form);
    v->addWidget(bbox);

    validate();
}

void AddRegisterDialog::validate()
{
    m_okBtn->setEnabled(true);
}

int AddRegisterDialog::startAddr() const
{
    return m_addrSpin->value();
}

int AddRegisterDialog::count() const
{
    return m_countSpin->value();
}
