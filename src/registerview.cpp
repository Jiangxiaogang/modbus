#include "registerview.h"
#include "modbusdefs.h"
#include "addregisterdialog.h"
#include "realtimedata.h"

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

// 只读单元格项：去掉可编辑标志，禁止双击进入编辑
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

    // TAB 顺序由 areaInfo() 的数组顺序决定：DO/DI/AO/AI
    for (int a = 0; a < 4; ++a)
    {
        setupTab(a);
        m_tabs->addTab(m_tables[a], areaInfo(a).name);
    }
}

void RegisterView::setupTab(int areaIndex)
{
    QTableWidget *t = new QTableWidget(0, 7, this);
    t->setContextMenuPolicy(Qt::CustomContextMenu);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::ExtendedSelection);
    t->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    t->verticalHeader()->setVisible(true);

    t->setHorizontalHeaderLabels({"寄存器名称", "寄存器地址", "数据类型", "原始值", "设定值", "写入", "状态"});
    t->horizontalHeader()->setStretchLastSection(true);
    t->setColumnWidth(ColName, 120);
    for (int c = ColAddr; c <= ColStatus; ++c)
        t->setColumnWidth(c, 80);
    t->horizontalHeader()->setHighlightSections(false);
    t->verticalHeader()->setHighlightSections(false);
    int hdrH = t->horizontalHeader()->height();
    if (hdrH > 0)
        t->verticalHeader()->setDefaultSectionSize(hdrH + 2);
    t->setItemDelegateForColumn(ColType, new TypeDelegate(areaIndex, this, t));

    connect(t, &QTableWidget::customContextMenuRequested, this,
            [this, t, areaIndex](const QPoint &pos){ showAreaMenu(t, areaIndex, pos); });
    connect(t, &QTableWidget::itemChanged, this, &RegisterView::onNameChanged);
    connect(t, &QTableWidget::itemDoubleClicked, this, &RegisterView::onItemDoubleClicked);
    m_tables[areaIndex] = t;
}

int RegisterView::areaOf(QTableWidget *table) const
{
    for (int a = 0; a < 4; ++a)
        if (m_tables[a] == table) return a;
    return -1;
}

int RegisterView::addRegisters(int areaIndex, int startAddr, int count)
{
    QTableWidget *t = m_tables[areaIndex];
    const AreaInfo &info = areaInfo(areaIndex);
    DataType def = (info.readFunc == 1 || info.readFunc == 2) ? TypeBIT : TypeU16;
    const int base = info.plcBase;

    // 现有地址集合，O(1) 查重，替代逐项线性查找
    QSet<int> existing;
    for (const RowData &rd : m_rows[areaIndex])
        existing.insert(rd.protoAddr);

    // 预生成待添加项：越界忽略，重复地址计入跳过数
    QList<RowData> adds;
    int dup = 0;
    for (int i = 0; i < count; ++i)
    {
        int proto = startAddr + i;
        if (proto < 0 || proto > 65535)
            continue;
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
        rd.type = def;
        adds.append(rd);
    }
    if (adds.isEmpty())
    {
        rebuildPlan(areaIndex);
        return dup;
    }

    // 批量插入期间暂停重绘，避免逐行刷新导致长时间无响应
    t->setUpdatesEnabled(false);
    t->viewport()->setUpdatesEnabled(false);

    const int oldCount = m_rows[areaIndex].size();
    if (oldCount == 0 || adds.first().protoAddr >= m_rows[areaIndex].last().protoAddr)
    {
        // 常见路径：新地址全部追加在尾部，一次性扩展行数后逐个填充新增行
        m_rows[areaIndex] += adds;
        t->setRowCount(oldCount + adds.size());
        for (int k = 0; k < adds.size(); ++k)
            fillRow(t, areaIndex, oldCount + k, adds[k], info);
    }
    else
    {
        // 中间插入：按升序逐项找位
        int pos = 0;
        for (const RowData &rd : adds)
        {
            while (pos < m_rows[areaIndex].size()
                   && m_rows[areaIndex][pos].protoAddr < rd.protoAddr)
                ++pos;
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

// 填充一行单元格与行内控件（类型下拉 / 写入按钮）
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
        // 0区/1区为位类型，固定 BIT，不可编辑
        QTableWidgetItem *typeItem = new QTableWidgetItem("bit");
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        typeItem->setTextAlignment(Qt::AlignCenter);
        t->setItem(r, ColType, typeItem);
    }
    else
    {
        // 3区/4区支持 uint16 / int16 选择：双击进入编辑，由 TypeDelegate 弹出下拉框
        QTableWidgetItem *typeItem = new QTableWidgetItem(rd.type == TypeS16 ? "s16" : "u16");
        typeItem->setTextAlignment(Qt::AlignCenter);
        t->setItem(r, ColType, typeItem);
    }

    QPushButton *wbtn = new QPushButton("写入", t);
    wbtn->setFlat(true);
    t->setCellWidget(r, ColWrite, wbtn);
    connect(wbtn, &QPushButton::clicked, this, [this, t, areaIndex, wbtn]{ doWrite(areaIndex, t, wbtn); });

    // 不可写区域：设定值不可编辑，写入按钮禁用
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
    QList<RegPlanItem> *items = new QList<RegPlanItem>();
    for (const RowData &rd : m_rows[areaIndex])
        items->append(RegPlanItem{rd.protoAddr, rd.type});
    emit planChanged(areaIndex, items);
}

// 类型编辑提交回调（由 TypeDelegate 在 setModelData 中调用）
void RegisterView::commitType(int area, int row, DataType tp)
{
    if (area < 0 || area > 3 || row < 0 || row >= m_rows[area].size()) return;
    m_rows[area][row].type = tp;
    rebuildPlan(area);
}

// ---------- TypeDelegate ----------

TypeDelegate::TypeDelegate(int area, RegisterView *owner, QObject *parent)
    : QStyledItemDelegate(parent), m_area(area), m_owner(owner)
{
}

QWidget *TypeDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &,
                                    const QModelIndex &) const
{
    QComboBox *cb = new QComboBox(parent);
    cb->addItems({"u16", "s16"});
    // 选中即提交并关闭编辑器
    TypeDelegate *self = const_cast<TypeDelegate *>(this);
    connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), self,
            [self, cb]{ self->commitData(cb); self->closeEditor(cb); });
    return cb;
}

void TypeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *cb = qobject_cast<QComboBox *>(editor);
    if (!cb) return;
    // 屏蔽信号，避免初始化选项时误触发提交关闭
    cb->blockSignals(true);
    cb->setCurrentIndex(qMax(0, cb->findText(index.model()->data(index, Qt::EditRole).toString())));
    cb->blockSignals(false);
}

void TypeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    QComboBox *cb = qobject_cast<QComboBox *>(editor);
    if (!cb) return;
    QString txt = cb->currentText();
    model->setData(index, txt, Qt::EditRole);
    m_owner->commitType(m_area, index.row(), txt == "s16" ? TypeS16 : TypeU16);
}

void RegisterView::onNameChanged(QTableWidgetItem *item)
{
    if (!item || item->column() != ColName) return;
    int area = areaOf(item->tableWidget());
    int row = item->row();
    if (area < 0 || row < 0 || row >= m_rows[area].size()) return;
    m_rows[area][row].name = item->text();
}

void RegisterView::doWrite(int area, QTableWidget *t, QPushButton *btn)
{
    int row = -1;
    for (int r = 0; r < t->rowCount(); ++r)
        if (t->cellWidget(r, ColWrite) == btn) { row = r; break; }
    if (area < 0 || area > 3 || row < 0 || row >= m_rows[area].size()) return;

    RowData &rd = m_rows[area][row];
    QTableWidgetItem *setItem = t->item(row, ColSet);
    if (!setItem) return;
    QString txt = setItem->text().trimmed();
    if (txt.isEmpty())
    {
        QMessageBox::warning(this, "写入失败",
            QString("%1 地址 %2\n设定值为空，请输入要写入的数值。")
                .arg(areaInfo(area).name).arg(addrText(rd.protoAddr)));
        return;
    }
    bool ok = false;
    qint64 val = txt.startsWith("0x", Qt::CaseInsensitive)
                 ? txt.mid(2).toLongLong(&ok, 16)
                 : txt.toLongLong(&ok, 10);
    if (!ok)
    {
        btn->setText("错误");
        QMessageBox::warning(this, "写入失败",
            QString("%1 地址 %2\n设定值 \"%3\" 不是有效的数值。")
                .arg(areaInfo(area).name).arg(addrText(rd.protoAddr)).arg(txt));
        return;
    }
    btn->setText("写入");
    emit writeRequested(area, rd.protoAddr, rd.type, val);
}

// 实时数据来自 RealtimeData 中转站转发的 dataChanged 信号
void RegisterView::onReadResult(const ReadPoint &pt)
{
    if (pt.areaIndex < 0 || pt.areaIndex > 3) return;
    QTableWidget *t = m_tables[pt.areaIndex];
    int row = findRow(t, pt.address);
    if (row < 0) return;

    QTableWidgetItem *st  = t->item(row, ColStatus);
    QTableWidgetItem *raw = t->item(row, ColRaw);
    st->setToolTip(QString());
    raw->setToolTip(QString());

    switch (pt.status)
    {
    case ReadOk:
        st->setText("有效");
        st->setTextColor(Qt::darkGreen);
        raw->setText(valueText(pt.value));
        break;

    case ReadTimeout:
        st->setText("超时");
        st->setTextColor(Qt::red);
        raw->setText("—");
        st->setToolTip(pt.errText);
        break;

    case ReadError:
        // Modbus 异常码与错误描述显示在状态列，原始值列无有效数据
        st->setText(QString("错误:0x%1 %2")
                    .arg((quint8)pt.errValue, 2, 16, QLatin1Char('0'))
                    .arg(pt.errText));
        st->setTextColor(Qt::red);
        st->setToolTip(pt.errText);
        raw->setText("—");
        break;

    default: // ReadInvalid
        st->setText("无效");
        st->setTextColor(Qt::red);
        raw->setText("");
        if (!pt.errText.isEmpty())
            st->setToolTip(pt.errText);
        break;
    }
}

void RegisterView::onWriteResult(int areaIndex, int protoAddr, bool ok, const QString &msg)
{
    if (areaIndex < 0 || areaIndex > 3) return;
    QTableWidget *t = m_tables[areaIndex];
    int row = findRow(t, protoAddr);
    if (row < 0) return;
    QPushButton *btn = qobject_cast<QPushButton *>(t->cellWidget(row, ColWrite));
    if (btn)
        btn->setText(ok ? "成功" : "失败");

    // 可写区(DO/AO)写入失败时弹框提示失败原因
    if (!ok && areaInfo(areaIndex).writable)
    {
        QMessageBox::warning(this, "写入失败",
            QString("%1 地址 %2\n原因：%3")
                .arg(areaInfo(areaIndex).name)
                .arg(addrText(protoAddr))
                .arg(msg.isEmpty() ? QString("未知错误") : msg));
    }
}

// 双击进入设定值编辑态时，将写入按钮文本还原为“写入”
void RegisterView::onItemDoubleClicked(QTableWidgetItem *item)
{
    if (!item || item->column() != ColSet) return;
    QTableWidget *t = item->tableWidget();
    if (!t) return;
    QPushButton *btn = qobject_cast<QPushButton *>(t->cellWidget(item->row(), ColWrite));
    if (btn)
        btn->setText("写入");
}

QString RegisterView::addrText(int protoAddr) const
{
    return m_hexAddr ? formatAddr(protoAddr) : QString::number(protoAddr);
}

void RegisterView::setAddrHex(bool hex)
{
    if (m_hexAddr == hex) return;
    m_hexAddr = hex;
    for (int a = 0; a < 4; ++a)
        for (int r = 0; r < m_rows[a].size(); ++r)
            if (QTableWidgetItem *it = m_tables[a]->item(r, ColAddr))
                it->setText(addrText(m_rows[a][r].protoAddr));
}

void RegisterView::setRealtimeData(RealtimeData *data)
{
    m_data = data;
}

QString RegisterView::valueText(qint64 value) const
{
    return m_hexValue ? QString("0x%1").arg((quint16)value, 4, 16, QLatin1Char('0'))
                      : QString::number(value);
}

void RegisterView::setValueHex(bool hex)
{
    if (m_hexValue == hex || !m_data) return;
    m_hexValue = hex;
    for (int a = 0; a < 4; ++a)
    {
        for (int r = 0; r < m_rows[a].size(); ++r)
        {
            QTableWidgetItem *it = m_tables[a]->item(r, ColRaw);
            if (it && m_data->value(a, m_rows[a][r].protoAddr).status == ReadOk)
                it->setText(valueText(m_data->value(a, m_rows[a][r].protoAddr).value));
        }
    }
}

int RegisterView::findRow(QTableWidget *table, int protoAddr) const
{
    int area = areaOf(table);
    if (area < 0) return -1;
    const QList<RowData> &rows = m_rows[area];
    for (int i = 0; i < rows.size(); ++i)
        if (rows[i].protoAddr == protoAddr) return i;
    return -1;
}

void RegisterView::showAreaMenu(QTableWidget *t, int area, const QPoint &pos)
{
    QMenu menu(this);
    QAction *addAct = menu.addAction("快速添加...");
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
    if (chosen == addAct)         onQuickAddInArea(area);
    else if (chosen == delAct)    onDeleteRowsInArea(area, t);
    else if (chosen == clearAct)  onClearArea(area, t);
    else if (chosen == hexAct)    setAddrHex(true);
    else if (chosen == decAct)    setAddrHex(false);
    else if (chosen == hexValAct) setValueHex(true);
    else if (chosen == decValAct) setValueHex(false);
}

void RegisterView::onQuickAddInArea(int area)
{
    AddRegisterDialog dlg(area, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        int skipped = addRegisters(area, dlg.startAddr(), dlg.count());
        if (skipped > 0)
            QMessageBox::information(this, "快速添加",
                QString("已跳过 %1 个与现有点位重复的地址。").arg(skipped));
    }
}

void RegisterView::onDeleteRowsInArea(int area, QTableWidget *t)
{
    QList<QTableWidgetItem *> sel = t->selectedItems();
    if (sel.isEmpty()) return;
    QList<int> rows;
    for (QTableWidgetItem *it : sel)
        if (!rows.contains(it->row())) rows.append(it->row());
    std::sort(rows.begin(), rows.end(), std::greater<int>()); // 从后往前删
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
        return;
    // 清空不可恢复，先确认
    if (QMessageBox::question(this, "清空列表",
                              QString("确定清空 %1 的全部点位吗？").arg(areaInfo(area).name),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes)
        return;

    t->setRowCount(0);           // 移除全部行及行内控件
    m_rows[area].clear();
    rebuildPlan(area);           // 同步空读取计划给工作线程，停止轮询该区
}
