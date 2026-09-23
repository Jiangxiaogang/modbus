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
    QTableWidget *t = new QTableWidget(0, 8, this);
    t->setContextMenuPolicy(Qt::CustomContextMenu);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::ExtendedSelection);
    t->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    t->verticalHeader()->setVisible(true);

    t->setHorizontalHeaderLabels({"寄存器名称", "寄存器地址", "数据类型", "字节序",
                                  "原始值", "设定值", "操作", "状态"});
    t->horizontalHeader()->setStretchLastSection(true);
    t->setColumnWidth(ColName, 120);
    for (int c = ColAddr; c <= ColStatus; ++c)
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
    t->setItem(r, ColStatus, makeReadOnlyItem("无效"));
    t->item(r, ColStatus)->setTextColor(Qt::red);
    t->setItem(r, ColRaw, makeReadOnlyItem(""));
    t->setItem(r, ColSet, new QTableWidgetItem(""));

    if (areaIndex == 0 || areaIndex == 1)
    {

        QTableWidgetItem *typeItem = new QTableWidgetItem(dataTypeText(TypeBIT));
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        typeItem->setTextAlignment(Qt::AlignCenter);
        t->setItem(r, ColType, typeItem);

        QTableWidgetItem *boItem = new QTableWidgetItem("-");
        boItem->setFlags(boItem->flags() & ~Qt::ItemIsEditable);
        boItem->setTextAlignment(Qt::AlignCenter);
        t->setItem(r, ColByteOrder, boItem);
    }
    else
    {

        QTableWidgetItem *typeItem = new QTableWidgetItem(dataTypeText(rd.type));
        typeItem->setTextAlignment(Qt::AlignCenter);
        t->setItem(r, ColType, typeItem);

        QTableWidgetItem *boItem = new QTableWidgetItem(byteOrderText(rd.byteOrder));
        boItem->setTextAlignment(Qt::AlignCenter);
        t->setItem(r, ColByteOrder, boItem);
    }

    QPushButton *wbtn = new QPushButton("写入", t);
    wbtn->setFlat(true);
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
        raw->setText("—");
        st->setToolTip(pt.errText);
        break;

    case ReadError:

        st->setText(QString("错误:0x%1 %2")
                    .arg((quint8)pt.errValue, 2, 16, QLatin1Char('0'))
                    .arg(pt.errText));
        st->setTextColor(Qt::red);
        st->setToolTip(pt.errText);
        raw->setText("—");
        break;

    default:
        st->setText("无效");
        st->setTextColor(Qt::red);
        raw->setText("");
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

void RegisterView::showAreaMenu(QTableWidget *t, int area, const QPoint &pos)
{
    QMenu menu(this);
    QAction *addAct = menu.addAction("添加点位...");
    QAction *delAct = menu.addAction("删除点位");
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

    QAction *chosen = menu.exec(t->viewport()->mapToGlobal(pos));
    if (chosen == addAct)
    {
        onAddPointsInArea(area);
    }
    else if (chosen == delAct)
    {
        onDeleteRowsInArea(area, t);
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
