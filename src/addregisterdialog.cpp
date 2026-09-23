#include "addregisterdialog.h"
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QVBoxLayout>

// 解析用户输入的寄存器地址：支持十进制与 0x/0X 开头的十六进制
static bool parseAddr(const QString &text, int *addr)
{
    QString s = text.trimmed();
    bool ok = false;
    int v = s.startsWith("0x", Qt::CaseInsensitive) ? s.mid(2).toInt(&ok, 16)
                                                    : s.toInt(&ok, 10);
    if (!ok || v < 0 || v > 65535)
        return false;
    if (addr) *addr = v;
    return true;
}

AddRegisterDialog::AddRegisterDialog(int, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("快速添加");
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
    connect(bbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // 地址非法时禁用确定按钮，避免传入越界地址
    connect(m_addrEdit, &QLineEdit::textChanged, this,
            [this]{ m_okBtn->setEnabled(parseAddr(m_addrEdit->text(), nullptr)); });
    m_okBtn->setEnabled(parseAddr(m_addrEdit->text(), nullptr));

    QVBoxLayout *v = new QVBoxLayout(this);
    v->addLayout(form);
    v->addWidget(bbox);
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
