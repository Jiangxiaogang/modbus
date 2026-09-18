#include "addregisterdialog.h"
#include "modbusdefs.h"
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QDialogButtonBox>

// 解析用户输入的寄存器地址：支持十进制与 0x/0X 开头的十六进制
static bool parseAddr(const QString &text, int *addr)
{
    QString s = text.trimmed();
    bool ok = false;
    int v = 0;
    if (s.startsWith("0x", Qt::CaseInsensitive))
        v = s.mid(2).toInt(&ok, 16);
    else
        v = s.toInt(&ok, 10);
    if (!ok || v < 0 || v > 65535)
        return false;
    if (addr) *addr = v;
    return true;
}

AddRegisterDialog::AddRegisterDialog(int areaIndex, QWidget *parent)
    : QDialog(parent), m_areaIndex(areaIndex)
{
    setWindowTitle(QString("快速添加"));
    setModal(true);

    m_addrEdit = new QLineEdit("0", this);
    m_addrEdit->setPlaceholderText("十进制或0x十六进制");

    m_countSpin = new QSpinBox(this);
    m_countSpin->setRange(1, 65535);
    m_countSpin->setValue(10);

    QFormLayout *form = new QFormLayout;
    form->addRow("寄存器地址:", m_addrEdit);
    form->addRow("寄存器数量:", m_countSpin);

    m_okBtn = new QPushButton("确定", this);
    QPushButton *cancel = new QPushButton("取消", this);
    QDialogButtonBox *bbox = new QDialogButtonBox;
    bbox->addButton(m_okBtn, QDialogButtonBox::AcceptRole);
    bbox->addButton(cancel, QDialogButtonBox::RejectRole);
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_addrEdit, SIGNAL(textChanged(QString)), this, SLOT(validate()));

    QVBoxLayout *v = new QVBoxLayout(this);
    v->addLayout(form);
    v->addWidget(bbox);

    validate();
}

void AddRegisterDialog::validate()
{
    // 地址非法时禁用确定按钮，避免传入越界地址
    m_okBtn->setEnabled(parseAddr(m_addrEdit->text(), 0));
}

int AddRegisterDialog::startAddr() const
{
    int v = 0;
    parseAddr(m_addrEdit->text(), &v);
    return v;
}

int AddRegisterDialog::count() const
{
    return m_countSpin->value();
}
