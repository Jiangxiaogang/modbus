#include "registerview.h"
#include "registerpoint.h"
#include "addregisterdialog.h"
#include "registerdata.h"
#include "modbuscontroller.h"

#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QComboBox>
#include <QPushButton>
#include <QMenu>
#include <QActionGroup>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QSet>
#include <algorithm>
#include <functional>
#include <cstring>

static QTableWidgetItem *makeReadOnlyItem(const QString &text)
{
    QTableWidgetItem *it = new QTableWidgetItem(text);
    it->setFlags(it->flags() & ~Qt::ItemIsEditable);
    return it;
}

/*! 取表格中已选中且不重复的行号（升序）。 */
static QList<int> selectedRowsOf(QTableWidget *t)
{
    QList<int> rows;
    for (QTableWidgetItem *it : t->selectedItems())
    {
        if (!rows.contains(it->row()))
        {
            rows.append(it->row());
        }
    }
    std::sort(rows.begin(), rows.end());
    return rows;
}

/*! CSV 字段转义：含逗号/引号/换行的字段加双引号，内部引号双写。 */
static QString csvEscape(const QString &text)
{
    if (!text.contains(QLatin1Char(',')) && !text.contains(QLatin1Char('"'))
        && !text.contains(QLatin1Char('\n')) && !text.contains(QLatin1Char('\r')))
    {
        return text;
    }
    QString out = text;
    out.replace(QLatin1String("\""), QLatin1String("\"\""));
    return QLatin1Char('"') + out + QLatin1Char('"');
}

/*! 解析 CSV 文本（支持双引号包裹、引号双写与字段内换行）。 */
static QList<QStringList> parseCsvText(const QString &text)
{
    QList<QStringList> rows;
    QStringList cur;
    QString field;
    bool inQuotes = false;
    for (int i = 0; i < text.size(); ++i)
    {
        const QChar c = text.at(i);
        if (inQuotes)
        {
            if (c == QLatin1Char('"'))
            {
                if (i + 1 < text.size() && text.at(i + 1) == QLatin1Char('"'))
                {
                    field.append(QLatin1Char('"'));
                    ++i;
                }
                else
                {
                    inQuotes = false;
                }
            }
            else
            {
                field.append(c);
            }
            continue;
        }

        if (c == QLatin1Char('"'))
        {
            inQuotes = true;
        }
        else if (c == QLatin1Char(','))
        {
            cur.append(field);
            field.clear();
        }
        else if (c == QLatin1Char('\n'))
        {
            cur.append(field);
            field.clear();
            rows.append(cur);
            cur.clear();
        }
        else if (c != QLatin1Char('\r'))
        {
            field.append(c);
        }
    }
    if (!field.isEmpty() || !cur.isEmpty())
    {
        cur.append(field);
        rows.append(cur);
    }
    return rows;
}

/*! 读取 CSV 字节：去 UTF-8 BOM；非 UTF-8 时按本地编码（中文 Windows 为 GBK）兜底。 */
static QString decodeCsvBytes(const QByteArray &bytes)
{
    QByteArray data = bytes;
    if (data.startsWith("\xEF\xBB\xBF"))
    {
        data.remove(0, 3);
    }
    QString text = QString::fromUtf8(data);
    if (text.contains(QChar::ReplacementCharacter))
    {
        text = QString::fromLocal8Bit(data);
    }
    return text;
}

/*! 解析十进制或 0x 前缀十六进制的整数。 */
static bool parseCsvNumber(const QString &text, int *value)
{
    QString s = text.trimmed();
    bool ok = false;
    int v = s.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)
            ? s.mid(2).toInt(&ok, 16)
            : s.toInt(&ok, 10);
    if (!ok || v < 0)
    {
        return false;
    }
    if (value)
    {
        *value = v;
    }
    return true;
}

RegisterView::RegisterView(QWidget *parent)
    : QWidget(parent)
    , m_hexAddr(true)
    , m_hexValue(false)
{
    QVBoxLayout *v = new QVBoxLayout(this);
    v->setContentsMargins(4,4,4,4);
    m_tabs = new QTabWidget(this);
    v->addWidget(m_tabs);

    for (int a = 0; a < 4; ++a)
    {
        setupTab(a);
        m_tabs->addTab(m_tables[a], areaInfo(a).name);
    }
}

void RegisterView::setupTab(int areaIndex)
{
    QTableWidget *t = new QTableWidget(0, 9, this);
    t->setContextMenuPolicy(Qt::CustomContextMenu);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::ExtendedSelection);
    t->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    t->verticalHeader()->setVisible(true);

    t->setHorizontalHeaderLabels({"寄存器名称", "寄存器地址", "功能码", "数据类型", "字节序",
                                  "原始值", "设定值", "操作", "状态"});
    t->horizontalHeader()->setStretchLastSection(true);
    t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    t->setColumnWidth(ColName, 80);
    t->setColumnWidth(ColAddr, 80);
    t->setColumnWidth(ColFunc, 60);
    for (int c = ColType; c <= ColStatus; ++c)
    {
        t->setColumnWidth(c, 80);
    }
    t->horizontalHeader()->setHighlightSections(false);
    t->verticalHeader()->setHighlightSections(false);
    int hdrH = t->horizontalHeader()->height();
    if (hdrH > 0)
    {
        t->verticalHeader()->setDefaultSectionSize(hdrH + 2);
    }
    t->setItemDelegateForColumn(ColType, new TypeDelegate(areaIndex, this, t));
    t->setItemDelegateForColumn(ColByteOrder, new ByteOrderDelegate(areaIndex, this, t));

    connect(t, &QTableWidget::customContextMenuRequested, this,
            [this, t, areaIndex](const QPoint &pos){ showAreaMenu(t, areaIndex, pos); });
    connect(t, &QTableWidget::itemChanged, this, &RegisterView::onNameChanged);
    connect(t, &QTableWidget::itemDoubleClicked, this, &RegisterView::onItemDoubleClicked);
    m_tables[areaIndex] = t;
}

int RegisterView::areaOf(QTableWidget *table) const
{
    for (int a = 0; a < 4; ++a)
    {
        if (m_tables[a] == table)
        {
            return a;
        }
    }
    return -1;
}

int RegisterView::addRegisters(int areaIndex, int startAddr, int count,
                               DataType type, ByteOrder byteOrder)
{
    QTableWidget *t = m_tables[areaIndex];
    const AreaInfo &info = areaInfo(areaIndex);
    const int base = info.plcBase;

    QSet<int> existing;
    for (const RowData &rd : m_rows[areaIndex])
    {
        existing.insert(rd.protoAddr);
    }

    const int step = is32BitType(type) ? 2 : 1;
    QList<RowData> adds;
    int dup = 0;
    for (int i = 0; i < count; ++i)
    {
        int proto = startAddr + i * step;
        if (proto < 0 || proto > 65535)
        {
            continue;
        }

        if (is32BitType(type) && proto > 65534)
        {
            continue;
        }
        if (existing.contains(proto))
        {
            ++dup;
            continue;
        }
        RowData rd;
        rd.name = (areaIndex == 0)
                  ? QString::number(proto + base).rightJustified(5, '0')
                  : QString::number(proto + base);
        rd.plcAddr = proto + base;
        rd.protoAddr = proto;
        rd.type = type;
        rd.byteOrder = byteOrder;
        adds.append(rd);
    }
    if (adds.isEmpty())
    {
        rebuildPlan(areaIndex);
        return dup;
    }

    t->setUpdatesEnabled(false);
    t->viewport()->setUpdatesEnabled(false);

    const int oldCount = m_rows[areaIndex].size();
    if (oldCount == 0 || adds.first().protoAddr >= m_rows[areaIndex].last().protoAddr)
    {

        m_rows[areaIndex] += adds;
        t->setRowCount(oldCount + adds.size());
        for (int k = 0; k < adds.size(); ++k)
        {
            fillRow(t, areaIndex, oldCount + k, adds[k], info);
        }
    }
    else
    {

        int pos = 0;
        for (const RowData &rd : adds)
        {
            while (pos < m_rows[areaIndex].size()
                   && m_rows[areaIndex][pos].protoAddr < rd.protoAddr)
            {
                ++pos;
            }
            m_rows[areaIndex].insert(pos, rd);
            t->insertRow(pos);
            fillRow(t, areaIndex, pos, rd, info);
            ++pos;
        }
    }

    t->setUpdatesEnabled(true);
    t->viewport()->setUpdatesEnabled(true);

    rebuildPlan(areaIndex);
    return dup;
}

void RegisterView::fillRow(QTableWidget *t, int areaIndex, int r,
                           const RowData &rd, const AreaInfo &info)
{
    QTableWidgetItem *name = new QTableWidgetItem(rd.name);
    name->setFlags(name->flags() | Qt::ItemIsEditable);
    t->setItem(r, ColName, name);
    t->setItem(r, ColAddr, makeReadOnlyItem(addrText(rd.protoAddr)));
    QTableWidgetItem *funcItem = makeReadOnlyItem(
        QString::number(info.readFunc).rightJustified(2, '0'));
    funcItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    t->setItem(r, ColFunc, funcItem);
    t->setItem(r, ColStatus, makeReadOnlyItem("无效"));
    t->item(r, ColStatus)->setTextColor(Qt::red);
    t->setItem(r, ColRaw, makeReadOnlyItem(""));
    t->setItem(r, ColSet, new QTableWidgetItem(""));

    if (areaIndex == 0 || areaIndex == 1)
    {

        QTableWidgetItem *typeItem = new QTableWidgetItem(dataTypeText(TypeBIT));
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        typeItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        t->setItem(r, ColType, typeItem);

        QTableWidgetItem *boItem = new QTableWidgetItem("-");
        boItem->setFlags(boItem->flags() & ~Qt::ItemIsEditable);
        boItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        t->setItem(r, ColByteOrder, boItem);
    }
    else
    {

        QTableWidgetItem *typeItem = new QTableWidgetItem(dataTypeText(rd.type));
        typeItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        t->setItem(r, ColType, typeItem);

        QTableWidgetItem *boItem = new QTableWidgetItem(byteOrderText(rd.byteOrder));
        boItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        t->setItem(r, ColByteOrder, boItem);
    }

    QPushButton *wbtn = new QPushButton("写入", t);
    wbtn->setFlat(true);
    wbtn->setStyleSheet("text-align: left;");
    t->setCellWidget(r, ColWrite, wbtn);
    connect(wbtn, &QPushButton::clicked, this, [this, t, areaIndex, wbtn]{ doWrite(areaIndex, t, wbtn); });

    if (!info.writable)
    {
        QTableWidgetItem *set = t->item(r, ColSet);
        set->setText("-");
        set->setFlags(set->flags() & ~Qt::ItemIsEditable);
        wbtn->setEnabled(false);
    }
}

void RegisterView::rebuildPlan(int areaIndex)
{
    if (!m_controller)
    {
        return;
    }
    QList<RegPlanItem> items;
    for (const RowData &rd : m_rows[areaIndex])
    {
        items.append(RegPlanItem{rd.protoAddr, rd.type, rd.byteOrder});
    }
    m_controller->setAreaPlan(areaIndex, items);
}

void RegisterView::commitType(int area, int row, DataType tp)
{
    if (area < 0 || area > 3 || row < 0 || row >= m_rows[area].size())
    {
        return;
    }
    m_rows[area][row].type = tp;

    if (!byteOrderValidFor(tp, m_rows[area][row].byteOrder))
    {
        m_rows[area][row].byteOrder = is32BitType(tp) ? ByteOrderABCD : ByteOrderAB;
        if (QTableWidgetItem *it = m_tables[area]->item(row, ColByteOrder))
        {
            it->setText(byteOrderText(m_rows[area][row].byteOrder));
        }
    }
    rebuildPlan(area);
}

DataType RegisterView::rowType(int area, int row) const
{
    if (area < 0 || area > 3 || row < 0 || row >= m_rows[area].size())
    {
        return TypeU16;
    }
    return m_rows[area][row].type;
}

void RegisterView::commitByteOrder(int area, int row, ByteOrder bo)
{
    if (area < 0 || area > 3 || row < 0 || row >= m_rows[area].size())
    {
        return;
    }
    m_rows[area][row].byteOrder = bo;
    rebuildPlan(area);
}

/*! 批量修改选中行的数据类型（仅遥测/遥调区）。
    新类型与原字节序不兼容时，按该类型的默认字节序重置。 */
void RegisterView::setRowsType(int area, const QList<int> &rows, DataType type)
{
    if (area < 2 || area > 3 || rows.isEmpty())
    {
        return;
    }
    QTableWidget *t = m_tables[area];
    bool changed = false;
    for (int r : rows)
    {
        if (r < 0 || r >= m_rows[area].size())
        {
            continue;
        }
        RowData &rd = m_rows[area][r];
        rd.type = type;
        if (!byteOrderValidFor(type, rd.byteOrder))
        {
            rd.byteOrder = is32BitType(type) ? ByteOrderABCD : ByteOrderAB;
        }
        if (QTableWidgetItem *it = t->item(r, ColType))
        {
            it->setText(dataTypeText(type));
        }
        if (QTableWidgetItem *it = t->item(r, ColByteOrder))
        {
            it->setText(byteOrderText(rd.byteOrder));
        }
        changed = true;
    }
    if (changed)
    {
        rebuildPlan(area);
    }
}

/*! 批量修改选中行的字节序（仅遥测/遥调区）。
    与某行类型不兼容的字节序会被跳过，该行保持不变。 */
void RegisterView::setRowsByteOrder(int area, const QList<int> &rows, ByteOrder bo)
{
    if (area < 2 || area > 3 || rows.isEmpty())
    {
        return;
    }
    QTableWidget *t = m_tables[area];
    bool changed = false;
    for (int r : rows)
    {
        if (r < 0 || r >= m_rows[area].size())
        {
            continue;
        }
        RowData &rd = m_rows[area][r];
        if (!byteOrderValidFor(rd.type, bo))
        {
            continue;
        }
        rd.byteOrder = bo;
        if (QTableWidgetItem *it = t->item(r, ColByteOrder))
        {
            it->setText(byteOrderText(bo));
        }
        changed = true;
    }
    if (changed)
    {
        rebuildPlan(area);
    }
}

TypeDelegate::TypeDelegate(int area, RegisterView *owner, QObject *parent)
    : QStyledItemDelegate(parent), m_area(area), m_owner(owner)
{
}

QWidget *TypeDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &,
                                    const QModelIndex &index) const
{
    QComboBox *cb = new QComboBox(parent);

    cb->addItems(dataTypeTextsFor(m_owner->rowType(m_area, index.row())));

    TypeDelegate *self = const_cast<TypeDelegate *>(this);
    connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), self,
            [self, cb]{ self->commitData(cb); self->closeEditor(cb); });
    return cb;
}

void TypeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *cb = qobject_cast<QComboBox *>(editor);
    if (!cb)
    {
        return;
    }

    cb->blockSignals(true);
    cb->setCurrentIndex(qMax(0, cb->findText(index.model()->data(index, Qt::EditRole).toString())));
    cb->blockSignals(false);
}

void TypeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    QComboBox *cb = qobject_cast<QComboBox *>(editor);
    if (!cb)
    {
        return;
    }
    QString txt = cb->currentText();
    model->setData(index, txt, Qt::EditRole);
    m_owner->commitType(m_area, index.row(), dataTypeFromText(txt));
}

ByteOrderDelegate::ByteOrderDelegate(int area, RegisterView *owner, QObject *parent)
    : QStyledItemDelegate(parent), m_area(area), m_owner(owner)
{
}

QWidget *ByteOrderDelegate::createEditor(QWidget *parent,
                                         const QStyleOptionViewItem &,
                                         const QModelIndex &index) const
{
    QComboBox *cb = new QComboBox(parent);

    cb->addItems(byteOrderTextsFor(m_owner->rowType(m_area, index.row())));

    ByteOrderDelegate *self = const_cast<ByteOrderDelegate *>(this);
    connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), self,
            [self, cb]{ self->commitData(cb); self->closeEditor(cb); });
    return cb;
}

void ByteOrderDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *cb = qobject_cast<QComboBox *>(editor);
    if (!cb)
    {
        return;
    }

    cb->blockSignals(true);
    cb->setCurrentIndex(qMax(0, cb->findText(index.model()->data(index, Qt::EditRole).toString())));
    cb->blockSignals(false);
}

void ByteOrderDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                     const QModelIndex &index) const
{
    QComboBox *cb = qobject_cast<QComboBox *>(editor);
    if (!cb)
    {
        return;
    }
    QString txt = cb->currentText();
    model->setData(index, txt, Qt::EditRole);
    m_owner->commitByteOrder(m_area, index.row(), byteOrderFromText(txt));
}

void RegisterView::onNameChanged(QTableWidgetItem *item)
{
    if (!item || item->column() != ColName)
    {
        return;
    }
    int area = areaOf(item->tableWidget());
    int row = item->row();
    if (area < 0 || row < 0 || row >= m_rows[area].size())
    {
        return;
    }
    m_rows[area][row].name = item->text();
}

void RegisterView::doWrite(int area, QTableWidget *t, QPushButton *btn)
{
    int row = -1;
    for (int r = 0; r < t->rowCount(); ++r)
    {
        if (t->cellWidget(r, ColWrite) == btn)
        {
            row = r;
            break;
        }
    }
    if (area < 0 || area > 3 || row < 0 || row >= m_rows[area].size())
    {
        return;
    }

    RowData &rd = m_rows[area][row];
    QTableWidgetItem *setItem = t->item(row, ColSet);
    if (!setItem)
    {
        return;
    }
    QString txt = setItem->text().trimmed();
    if (txt.isEmpty())
    {
        QMessageBox::warning(this, "写入失败",
            QString("%1 地址 %2\n设定值为空，请输入要写入的数值。")
                .arg(areaInfo(area).name).arg(addrText(rd.protoAddr)));
        return;
    }
    bool ok = false;
    qint64 val = 0;
    if (rd.type == TypeF32)
    {

        float f = txt.toFloat(&ok);
        quint32 bits = 0;
        std::memcpy(&bits, &f, sizeof(bits));
        val = (qint64)bits;
    }
    else
    {
        val = txt.startsWith("0x", Qt::CaseInsensitive)
              ? txt.mid(2).toLongLong(&ok, 16)
              : txt.toLongLong(&ok, 10);
    }
    if (!ok)
    {
        btn->setText("错误");
        QMessageBox::warning(this, "写入失败",
            QString("%1 地址 %2\n设定值 \"%3\" 不是有效的数值。")
                .arg(areaInfo(area).name).arg(addrText(rd.protoAddr)).arg(txt));
        return;
    }
    btn->setText("写入");
    if (m_controller)
    {
        m_controller->writeRegister(area, rd.protoAddr, rd.type, rd.byteOrder, val);
    }
}

void RegisterView::onReadResult(const ReadPoint &pt)
{
    if (pt.areaIndex < 0 || pt.areaIndex > 3)
    {
        return;
    }
    QTableWidget *t = m_tables[pt.areaIndex];
    int row = findRow(t, pt.address);
    if (row < 0)
    {
        return;
    }

    QTableWidgetItem *st  = t->item(row, ColStatus);
    QTableWidgetItem *raw = t->item(row, ColRaw);
    st->setToolTip(QString());
    raw->setToolTip(QString());

    switch (pt.status)
    {
    case ReadOk:
        st->setText("有效");
        st->setTextColor(Qt::darkGreen);
        raw->setText(valueText(m_rows[pt.areaIndex][row].type, pt.value));
        break;

    case ReadTimeout:
        st->setText("超时");
        st->setTextColor(Qt::red);
        st->setToolTip(pt.errText);
        break;

    case ReadError:
        st->setText(QString("错误:0x%1(%2)")
                    .arg((quint8)pt.errValue, 2, 16, QLatin1Char('0'))
                    .arg(pt.errText));
        st->setTextColor(Qt::red);
        st->setToolTip(pt.errText);
        break;

    default:
        st->setText("无效");
        st->setTextColor(Qt::red);
        if (!pt.errText.isEmpty())
        {
            st->setToolTip(pt.errText);
        }
        break;
    }
}

void RegisterView::onWriteResult(int areaIndex, int protoAddr, bool ok, const QString &msg)
{
    if (areaIndex < 0 || areaIndex > 3)
    {
        return;
    }
    QTableWidget *t = m_tables[areaIndex];
    int row = findRow(t, protoAddr);
    if (row < 0)
    {
        return;
    }
    QPushButton *btn = qobject_cast<QPushButton *>(t->cellWidget(row, ColWrite));
    if (btn)
    {
        btn->setText(ok ? "成功" : "失败");
    }

    if (!ok && areaInfo(areaIndex).writable)
    {
        QMessageBox::warning(this, "写入失败",
            QString("%1 地址 %2\n原因：%3")
                .arg(areaInfo(areaIndex).name)
                .arg(addrText(protoAddr))
                .arg(msg.isEmpty() ? QString("未知错误") : msg));
    }
}

void RegisterView::onItemDoubleClicked(QTableWidgetItem *item)
{
    if (!item || item->column() != ColSet)
    {
        return;
    }
    QTableWidget *t = item->tableWidget();
    if (!t)
    {
        return;
    }
    QPushButton *btn = qobject_cast<QPushButton *>(t->cellWidget(item->row(), ColWrite));
    if (btn)
    {
        btn->setText("写入");
    }
}

QString RegisterView::addrText(int protoAddr) const
{
    return m_hexAddr ? formatAddr(protoAddr) : QString::number(protoAddr);
}

void RegisterView::setAddrHex(bool hex)
{
    if (m_hexAddr == hex)
    {
        return;
    }
    m_hexAddr = hex;
    for (int a = 0; a < 4; ++a)
    {
        for (int r = 0; r < m_rows[a].size(); ++r)
        {
            if (QTableWidgetItem *it = m_tables[a]->item(r, ColAddr))
            {
                it->setText(addrText(m_rows[a][r].protoAddr));
            }
        }
    }
}

void RegisterView::setRegisterData(RegisterData *data)
{
    m_data = data;
}

void RegisterView::setController(IModbusController *controller)
{
    m_controller = controller;
}

QString RegisterView::valueText(DataType type, qint64 value) const
{

    if (type == TypeF32)
    {
        quint32 bits = (quint32)value;
        float f;
        std::memcpy(&f, &bits, sizeof(f));
        return QString::number(f, 'g', 7);
    }
    if (m_hexValue)
    {
        if (is32BitType(type))
        {
            return QString("0x%1").arg((quint32)value, 8, 16, QLatin1Char('0'));
        }
        return QString("0x%1").arg((quint16)value, 4, 16, QLatin1Char('0'));
    }
    return QString::number(value);
}

void RegisterView::setValueHex(bool hex)
{
    if (m_hexValue == hex || !m_data)
    {
        return;
    }
    m_hexValue = hex;
    for (int a = 0; a < 4; ++a)
    {
        for (int r = 0; r < m_rows[a].size(); ++r)
        {
            QTableWidgetItem *it = m_tables[a]->item(r, ColRaw);
            if (it && m_data->value(a, m_rows[a][r].protoAddr).status == ReadOk)
            {
                it->setText(valueText(m_rows[a][r].type,
                                      m_data->value(a, m_rows[a][r].protoAddr).value));
            }
        }
    }
}

int RegisterView::findRow(QTableWidget *table, int protoAddr) const
{
    int area = areaOf(table);
    if (area < 0)
    {
        return -1;
    }
    const QList<RowData> &rows = m_rows[area];
    for (int i = 0; i < rows.size(); ++i)
    {
        if (rows[i].protoAddr == protoAddr)
        {
            return i;
        }
    }
    return -1;
}

/*! 批量追加导入的点位：按协议地址去重并保持升序，最后整表重建。
    返回因地址重复被跳过的数量。 */
int RegisterView::addImportedRows(int area, const QList<RowData> &rows)
{
    if (area < 0 || area > 3 || rows.isEmpty())
    {
        return 0;
    }

    QSet<int> existing;
    for (const RowData &rd : m_rows[area])
    {
        existing.insert(rd.protoAddr);
    }

    int skipped = 0;
    const int oldCount = m_rows[area].size();
    for (const RowData &rd : rows)
    {
        if (existing.contains(rd.protoAddr))
        {
            ++skipped;
            continue;
        }
        existing.insert(rd.protoAddr);
        m_rows[area].append(rd);
    }
    if (m_rows[area].size() == oldCount)
    {
        return skipped;
    }

    QList<RowData> &list = m_rows[area];
    std::sort(list.begin(), list.end(),
              [](const RowData &a, const RowData &b){ return a.protoAddr < b.protoAddr; });

    QTableWidget *t = m_tables[area];
    t->setRowCount(0);
    t->setRowCount(list.size());
    const AreaInfo &info = areaInfo(area);
    for (int r = 0; r < list.size(); ++r)
    {
        fillRow(t, area, r, list.at(r), info);
    }
    rebuildPlan(area);
    return skipped;
}

/*! 导入 CSV 点表：列依次为 寄存器名称,寄存器地址,功能码,数据类型,字节序。
    按功能码 1/2/3/4 分发到 DO/DI/AO/AI 四个页；地址重复的点位跳过。 */
void RegisterView::onImportPoints()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "导入点表", QString(), "CSV 文件 (*.csv);;所有文件 (*)");
    if (path.isEmpty())
    {
        return;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "导入点表",
                             QString("无法打开文件：\n%1\n%2").arg(path, f.errorString()));
        return;
    }
    const QString text = decodeCsvBytes(f.readAll());
    f.close();

    const QList<QStringList> lines = parseCsvText(text);
    QList<RowData> pending[4];
    int errors = 0;
    bool firstLine = true;

    for (const QStringList &fields : lines)
    {
        if (fields.size() == 1 && fields.first().trimmed().isEmpty())
        {
            continue;   // 空行
        }
        if (firstLine)
        {
            firstLine = false;
            if (fields.value(0).trimmed() == QString("寄存器名称"))
            {
                continue;   // 表头
            }
        }
        if (fields.size() < 4)
        {
            ++errors;
            continue;
        }

        int func = 0;
        if (!parseCsvNumber(fields.value(2), &func))
        {
            ++errors;
            continue;
        }
        int area = -1;
        switch (func)
        {
        case 1: area = 0; break;
        case 2: area = 1; break;
        case 3: area = 2; break;
        case 4: area = 3; break;
        default: break;
        }
        if (area < 0)
        {
            ++errors;
            continue;
        }

        int proto = 0;
        if (!parseCsvNumber(fields.value(1), &proto) || proto > 65535)
        {
            ++errors;
            continue;
        }

        RowData rd;
        rd.protoAddr = proto;
        rd.plcAddr   = proto + areaInfo(area).plcBase;
        rd.type      = TypeBIT;
        rd.byteOrder = ByteOrderAB;

        if (area > 1)   // AO/AI：解析数据类型与字节序
        {
            const QString typeText = fields.value(3).trimmed().toUpper();
            const DataType type = dataTypeFromText(typeText);
            if (dataTypeText(type).compare(typeText, Qt::CaseInsensitive) != 0
                || type == TypeBIT)
            {
                ++errors;
                continue;
            }
            if (is32BitType(type) && proto > 65534)
            {
                ++errors;
                continue;
            }
            rd.type = type;

            const QString boText = fields.value(4).trimmed().toUpper();
            if (!boText.isEmpty() && boText != QLatin1String("-"))
            {
                const ByteOrder bo = byteOrderFromText(boText);
                if (byteOrderText(bo).compare(boText, Qt::CaseInsensitive) != 0)
                {
                    ++errors;
                    continue;
                }
                rd.byteOrder = bo;
            }
            if (!byteOrderValidFor(rd.type, rd.byteOrder))
            {
                rd.byteOrder = is32BitType(rd.type) ? ByteOrderABCD : ByteOrderAB;
            }
        }

        rd.name = fields.value(0).trimmed();
        if (rd.name.isEmpty())
        {
            rd.name = (area == 0) ? QString::number(rd.plcAddr).rightJustified(5, QLatin1Char('0'))
                                  : QString::number(rd.plcAddr);
        }
        pending[area].append(rd);
    }

    int added = 0;
    int skipped = 0;
    for (int a = 0; a < 4; ++a)
    {
        if (pending[a].isEmpty())
        {
            continue;
        }
        const int sk = addImportedRows(a, pending[a]);
        added   += pending[a].size() - sk;
        skipped += sk;
    }

    if (added == 0 && skipped == 0 && errors == 0)
    {
        QMessageBox::information(this, "导入点表", "文件中没有可导入的点位。");
        return;
    }

    QString msg = QString("导入完成：新增 %1 个点位").arg(added);
    if (skipped > 0)
    {
        msg += QString("，跳过重复 %1 个").arg(skipped);
    }
    if (errors > 0)
    {
        msg += QString("，忽略无法解析 %1 行").arg(errors);
    }
    QMessageBox::information(this, "导入点表", msg);
}

/*! 导出 CSV 点表：把四个页的全部点位按 功能码 写入同一个文件。 */
void RegisterView::onExportPoints()
{
    QString path = QFileDialog::getSaveFileName(
        this, "导出点表",
        QString("点表_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "CSV 文件 (*.csv)");
    if (path.isEmpty())
    {
        return;
    }
    if (!path.endsWith(QLatin1String(".csv"), Qt::CaseInsensitive))
    {
        path += QLatin1String(".csv");
    }

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QMessageBox::warning(this, "导出点表",
                             QString("无法写入文件：\n%1\n%2").arg(path, f.errorString()));
        return;
    }

    QTextStream ts(&f);
    ts.setCodec("UTF-8");
    ts.setGenerateByteOrderMark(true);
    ts << QString("寄存器名称,寄存器地址,功能码,数据类型,字节序\r\n");

    int count = 0;
    for (int a = 0; a < 4; ++a)
    {
        const QString funcText = QString::number(areaInfo(a).readFunc).rightJustified(2, QLatin1Char('0'));
        const bool bitArea = (a <= 1);
        for (const RowData &rd : m_rows[a])
        {
            ts << csvEscape(rd.name) << QLatin1Char(',')
               << formatAddr(rd.protoAddr) << QLatin1Char(',')
               << funcText << QLatin1Char(',')
               << (bitArea ? dataTypeText(TypeBIT) : dataTypeText(rd.type)) << QLatin1Char(',')
               << (bitArea ? QString("-") : byteOrderText(rd.byteOrder))
               << QString("\r\n");
            ++count;
        }
    }
    ts.flush();
    f.close();

    QMessageBox::information(this, "导出点表",
                             QString("已导出 %1 个点位到：\n%2").arg(count).arg(path));
}

void RegisterView::showAreaMenu(QTableWidget *t, int area, const QPoint &pos)
{
    QMenu menu(this);
    QAction *addAct = menu.addAction("添加点位...");
    QAction *delAct = menu.addAction("删除点位");
    delAct->setEnabled(!t->selectedItems().isEmpty());
    QAction *importAct = menu.addAction("导入点表...");
    QAction *exportAct = menu.addAction("导出点表...");

    menu.addSeparator();
    QAction *clearAct = menu.addAction("清空列表");
    clearAct->setEnabled(t->rowCount() > 0);

    menu.addSeparator();
    QMenu *fmtMenu = menu.addMenu("地址格式");
    QActionGroup *fmtGroup = new QActionGroup(&menu);
    QAction *hexAct = fmtMenu->addAction("16进制");
    QAction *decAct = fmtMenu->addAction("10进制");
    for (QAction *a : {hexAct, decAct}) { a->setCheckable(true); a->setActionGroup(fmtGroup); }
    hexAct->setChecked(m_hexAddr);
    decAct->setChecked(!m_hexAddr);

    QMenu *valMenu = menu.addMenu("数值格式");
    QActionGroup *valGroup = new QActionGroup(&menu);
    QAction *hexValAct = valMenu->addAction("16进制");
    QAction *decValAct = valMenu->addAction("10进制");
    for (QAction *a : {hexValAct, decValAct}) { a->setCheckable(true); a->setActionGroup(valGroup); }
    hexValAct->setChecked(m_hexValue);
    decValAct->setChecked(!m_hexValue);

    // 遥测/遥调：有选中行时，在菜单末尾追加批量修改项。
    QList<int> selRows;
    bool batchType = false;
    bool batchByteOrder = false;
    DataType  batchTypeValue = TypeU16;
    ByteOrder batchByteOrderValue = ByteOrderAB;

    if (area == 2 || area == 3)
    {
        selRows = selectedRowsOf(t);
    }
    if (!selRows.isEmpty())
    {
        bool has16 = false;
        bool has32 = false;
        for (int r : selRows)
        {
            if (r >= 0 && r < m_rows[area].size())
            {
                if (is32BitType(m_rows[area][r].type))
                {
                    has32 = true;
                }
                else
                {
                    has16 = true;
                }
            }
        }

        // 仅当选中行宽度一致（全 16 位或全 32 位）时才显示这两个菜单；
        // 混合 16/32 位时都不显示，避免跨宽度批量修改。
        if (has16 != has32)
        {
            menu.addSeparator();

            QMenu *typeMenu = menu.addMenu("数据类型");
            for (const QString &txt : dataTypeTextsFor(has32 ? TypeU32 : TypeU16))
            {
                QAction *a = typeMenu->addAction(txt);
                connect(a, &QAction::triggered, this,
                        [&batchType, &batchTypeValue, txt]{
                            batchType = true;
                            batchTypeValue = dataTypeFromText(txt);
                        });
            }

            QMenu *boMenu = menu.addMenu("字节序");
            for (const QString &txt : byteOrderTextsFor(has32 ? TypeU32 : TypeU16))
            {
                QAction *a = boMenu->addAction(txt);
                connect(a, &QAction::triggered, this,
                        [&batchByteOrder, &batchByteOrderValue, txt]{
                            batchByteOrder = true;
                            batchByteOrderValue = byteOrderFromText(txt);
                        });
            }
        }
    }

    QAction *chosen = menu.exec(t->viewport()->mapToGlobal(pos));
    if (chosen == addAct)
    {
        onAddPointsInArea(area);
    }
    else if (chosen == delAct)
    {
        onDeleteRowsInArea(area, t);
    }
    else if (chosen == importAct)
    {
        onImportPoints();
    }
    else if (chosen == exportAct)
    {
        onExportPoints();
    }
    else if (chosen == clearAct)
    {
        onClearArea(area, t);
    }
    else if (chosen == hexAct)
    {
        setAddrHex(true);
    }
    else if (chosen == decAct)
    {
        setAddrHex(false);
    }
    else if (chosen == hexValAct)
    {
        setValueHex(true);
    }
    else if (chosen == decValAct)
    {
        setValueHex(false);
    }
    if (batchType)
    {
        setRowsType(area, selRows, batchTypeValue);
    }
    if (batchByteOrder)
    {
        setRowsByteOrder(area, selRows, batchByteOrderValue);
    }
}

void RegisterView::onAddPointsInArea(int area)
{
    AddRegisterDialog dlg(area, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        int skipped = addRegisters(area, dlg.startAddr(), dlg.count(),
                                   dlg.dataType(), dlg.byteOrder());
        if (skipped > 0)
        {
            QMessageBox::information(this, "添加点位",
                QString("已跳过 %1 个与现有点位重复的地址。").arg(skipped));
        }
    }
}

void RegisterView::onDeleteRowsInArea(int area, QTableWidget *t)
{
    QList<QTableWidgetItem *> sel = t->selectedItems();
    if (sel.isEmpty())
    {
        return;
    }
    QList<int> rows;
    for (QTableWidgetItem *it : sel)
    {
        if (!rows.contains(it->row()))
        {
            rows.append(it->row());
        }
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows)
    {
        t->removeRow(r);
        m_rows[area].removeAt(r);
    }
    rebuildPlan(area);
}

void RegisterView::onClearArea(int area, QTableWidget *t)
{
    if (area < 0 || !t || t->rowCount() == 0)
    {
        return;
    }

    if (QMessageBox::question(this, "清空列表",
                              QString("确定清空 %1 的全部点位吗？").arg(areaInfo(area).name),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }

    t->setRowCount(0);
    m_rows[area].clear();
    rebuildPlan(area);
}
