#include "addregisterdialog.h"
#include "modbusdefs.h"
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QIntValidator>
#include <QDialogButtonBox>

AddRegisterDialog::AddRegisterDialog(int areaIndex, QWidget* parent)
    : QDialog(parent), m_areaIndex(areaIndex)
{
    const AreaInfo& info = areaInfo(areaIndex);
    setWindowTitle(QString("快速添加 - %1").arg(info.name));
    setModal(true);

    m_addrEdit = new QLineEdit(QString::number(info.plcBase), this);
    m_addrEdit->setValidator(new QIntValidator(1, 65535, this));

    m_countSpin = new QSpinBox(this);
    m_countSpin->setRange(1, 2000);
    m_countSpin->setValue(10);

    QFormLayout* form = new QFormLayout;
    form->addRow("起始 PLC 地址", m_addrEdit);
    form->addRow("数量", m_countSpin);

    m_okBtn = new QPushButton("确定", this);
    QPushButton* cancel = new QPushButton("取消", this);
    QDialogButtonBox* bbox = new QDialogButtonBox;
    bbox->addButton(m_okBtn, QDialogButtonBox::AcceptRole);
    bbox->addButton(cancel, QDialogButtonBox::RejectRole);
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_addrEdit, SIGNAL(textChanged(QString)), this, SLOT(validate()));

    QVBoxLayout* v = new QVBoxLayout(this);
    v->addLayout(form);
    v->addWidget(bbox);

    validate();
}

void AddRegisterDialog::validate()
{
    bool ok = false;
    int v = m_addrEdit->text().toInt(&ok);
    const AreaInfo& info = areaInfo(m_areaIndex);
    int maxPlc = info.plcBase + 9998;  // 每区最多 9999 个
    bool valid = ok && v >= info.plcBase && v <= maxPlc;
    m_okBtn->setEnabled(valid);
}

int AddRegisterDialog::startPlc() const
{
    return m_addrEdit->text().toInt();
}

int AddRegisterDialog::count() const
{
    return m_countSpin->value();
}
