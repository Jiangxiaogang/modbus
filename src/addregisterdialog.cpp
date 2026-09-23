#include "addregisterdialog.h"
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QVBoxLayout>

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

AddRegisterDialog::AddRegisterDialog(int areaIndex, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("添加点位");
    setModal(true);

    const AreaInfo &info = areaInfo(areaIndex);
    const bool bitArea = (info.readFunc == 1 || info.readFunc == 2);

    m_addrEdit = new QLineEdit("0", this);
    m_addrEdit->setPlaceholderText("十进制或0x十六进制");

    m_countSpin = new QSpinBox(this);
    m_countSpin->setRange(1, 65535);
    m_countSpin->setValue(10);

    m_typeCombo = new QComboBox(this);
    m_byteOrderCombo = new QComboBox(this);
    if (bitArea)
    {

        m_typeCombo->addItem(dataTypeText(TypeBIT));
        m_typeCombo->setEnabled(false);
        m_byteOrderCombo->addItem("-");
        m_byteOrderCombo->setEnabled(false);
    }
    else
    {
        m_typeCombo->addItems(dataTypeTextsAll());
        refreshByteOrderOptions();

        connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int){ refreshByteOrderOptions(); });
    }

    QFormLayout *form = new QFormLayout;
    form->addRow("寄存器地址:", m_addrEdit);
    form->addRow("寄存器数量:", m_countSpin);
    form->addRow("数据类型:", m_typeCombo);
    form->addRow("字节序:", m_byteOrderCombo);

    m_okBtn = new QPushButton("确定", this);
    QPushButton *cancel = new QPushButton("取消", this);
    QDialogButtonBox *bbox = new QDialogButtonBox;
    bbox->addButton(m_okBtn, QDialogButtonBox::AcceptRole);
    bbox->addButton(cancel, QDialogButtonBox::RejectRole);
    connect(bbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);

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

DataType AddRegisterDialog::dataType() const
{
    return dataTypeFromText(m_typeCombo->currentText());
}

ByteOrder AddRegisterDialog::byteOrder() const
{
    return byteOrderFromText(m_byteOrderCombo->currentText());
}

void AddRegisterDialog::refreshByteOrderOptions()
{
    const QStringList opts = byteOrderTextsFor(dataType());
    const QString cur = m_byteOrderCombo->currentText();
    m_byteOrderCombo->blockSignals(true);
    m_byteOrderCombo->clear();
    m_byteOrderCombo->addItems(opts);
    const int i = m_byteOrderCombo->findText(cur);
    m_byteOrderCombo->setCurrentIndex(i >= 0 ? i : 0);
    m_byteOrderCombo->blockSignals(false);
}
