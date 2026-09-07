#include "registerview.h"
#include "modbusdefs.h"
#include "addregisterdialog.h"

#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QComboBox>
#include <QPushButton>
#include <QMenu>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>

RegisterView::RegisterView(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *v = new QVBoxLayout(this);
    v->setMargin(4);
    m_tabs = new QTabWidget(this);
    v->addWidget(m_tabs);

    for (int a = 0; a < 4; ++a)
    {
        setupTab(a);
        m_tabs->addTab(m_tables[a], areaInfo(a).name);
    }
    connect(m_tabs, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));
}

void RegisterView::setupTab(int areaIndex)
{
    QTableWidget *t = new QTableWidget(0, 7, this);
    t->setContextMenuPolicy(Qt::CustomContextMenu);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::ExtendedSelection);
    t->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    t->verticalHeader()->setVisible(true);

    QStringList headers;
    headers << "寄存器名称" << "寄存器地址" << "数据类型" << "原始值" << "设定值" << "写入" <<  "状态";
    t->setHorizontalHeaderLabels(headers);
    t->horizontalHeader()->setStretchLastSection(true);
    t->setColumnWidth(ColName, 120);
    t->setColumnWidth(ColAddr, 80);
    t->setColumnWidth(ColStatus, 80);
    t->setColumnWidth(ColType, 80);
    t->setColumnWidth(ColRaw, 80);
    t->setColumnWidth(ColSet, 80);
    t->setColumnWidth(ColWrite, 80);
    t->horizontalHeader()->setHighlightSections(false);
    t->verticalHeader()->setHighlightSections(false);
    int hdrH = t->horizontalHeader()->height();
    if (hdrH > 0)
    {
        t->verticalHeader()->setDefaultSectionSize(hdrH + 2);
    }

    connect(t, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onCustomContextMenu(QPoint)));
    connect(t, SIGNAL(itemChanged(QTableWidgetItem *)),this, SLOT(onNameChanged(QTableWidgetItem *)));
    m_tables[areaIndex] = t;
}

void RegisterView::onTabChanged(int /*index*/)
{
    // 切换页时无需操作
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

    int base = info.plcBase;
    int skipped = 0;
    t->setSortingEnabled(false);
    for (int i = 0; i < count; ++i)
    {
        int proto = startAddr + i;
        if (proto < 0 || proto > 65535)
            continue;
        // 快速添加时跳过本区已存在的地址，避免重复点位
        if (findRow(t, proto) >= 0)
        {
            ++skipped;
            continue;
        }
        int plc = proto + base;

        RowData rd;
        rd.name = (areaIndex == 0)
                  ? QString::number(plc).rightJustified(5, '0')
                  : QString::number(plc);
        rd.plcAddr = plc;
        rd.protoAddr = proto;
        rd.type = def;
        m_rows[areaIndex].append(rd);

        int r = t->rowCount();
        t->insertRow(r);

        QTableWidgetItem *name = new QTableWidgetItem(rd.name);
        name->setFlags(name->flags() | Qt::ItemIsEditable);
        t->setItem(r, ColName, name);
        t->setItem(r, ColAddr, new QTableWidgetItem(formatAddr(proto)));
        t->setItem(r, ColStatus, new QTableWidgetItem("无效"));
        t->item(r, ColStatus)->setTextColor(Qt::red);
        t->setItem(r, ColRaw, new QTableWidgetItem(""));
        t->setItem(r, ColSet, new QTableWidgetItem(""));

        if (areaIndex == 0 || areaIndex == 1)
        {
            // 0区/1区为位类型，固定 BIT，不提供下拉
            QTableWidgetItem *typeItem = new QTableWidgetItem("bit");
            typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
            typeItem->setTextAlignment(Qt::AlignCenter);
            t->setItem(r, ColType, typeItem);
        }
        else
        {
            // 3区/4区支持 uint16 / int16 选择
            QComboBox *typeCombo = new QComboBox(t);
            typeCombo->addItems(QStringList() << "uint16" << "int16");
            typeCombo->setCurrentIndex(def == TypeS16 ? 1 : 0);
            typeCombo->setProperty("area", areaIndex);
            typeCombo->setProperty("row", r);
            // 去掉组合框与下拉箭头按钮的 3D 边框，融入表格
            typeCombo->setStyleSheet(
                "QComboBox { border: none; background: transparent; }"
                "QComboBox::drop-down { border: none; background: transparent; }"
            );
            t->setCellWidget(r, ColType, typeCombo);
            connect(typeCombo, SIGNAL(currentIndexChanged(int)),
                    this, SLOT(onTypeChanged(int)));
        }

        QPushButton *wbtn = new QPushButton("写入", t);
        wbtn->setProperty("area", areaIndex);
        wbtn->setProperty("row", r);
        wbtn->setFlat(true);
        t->setCellWidget(r, ColWrite, wbtn);
        connect(wbtn, SIGNAL(clicked()), this, SLOT(onWriteClicked()));

        // 不可写区域：设定值不可编辑，写入按钮禁用
        if (!info.writable)
        {
            QTableWidgetItem *set = t->item(r, ColSet);
            if (set) set->setFlags(set->flags() & ~Qt::ItemIsEditable);
            wbtn->setEnabled(false);
        }
    }
    t->setSortingEnabled(false);
    rebuildPlan(areaIndex);
    return skipped;
}

void RegisterView::rebuildPlan(int areaIndex)
{
    QList<RegPlanItem> *items = new QList<RegPlanItem>();
    foreach (const RowData &rd, m_rows[areaIndex])
    {
        RegPlanItem it;
        it.address = rd.protoAddr;
        it.type = rd.type;
        items->append(it);
    }
    emit planChanged(areaIndex, items);
}

void RegisterView::onTypeChanged(int /*index*/)
{
    QComboBox *combo = qobject_cast<QComboBox *>(sender());
    if (!combo) return;
    // 注意: setCellWidget 会把控件重设父对象为 viewport，
    // 不能靠 parent() 反查表格，改用创建时写入的动态属性
    int area = combo->property("area").toInt();
    int row = combo->property("row").toInt();
    if (area < 0 || area > 3 || row < 0 || row >= m_rows[area].size()) return;

    QString txt = combo->currentText();
    DataType tp = (txt == "int16") ? TypeS16 : TypeU16;
    m_rows[area][row].type = tp;
    rebuildPlan(area);
}

void RegisterView::onNameChanged(QTableWidgetItem *item)
{
    if (!item || item->column() != ColName) return;
    QTableWidget *t = item->tableWidget();
    int area = areaOf(t);
    int row = item->row();
    if (area < 0 || row < 0 || row >= m_rows[area].size()) return;
    m_rows[area][row].name = item->text();
}

void RegisterView::onWriteClicked()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;
    // 注意: setCellWidget 会把控件重设父对象为 viewport，
    // 不能靠 parent() 反查表格，改用创建时写入的动态属性
    int area = btn->property("area").toInt();
    int row = btn->property("row").toInt();
    if (area < 0 || area > 3 || row < 0 || row >= m_rows[area].size()) return;
    QTableWidget *t = m_tables[area];

    RowData &rd = m_rows[area][row];
    QTableWidgetItem *setItem = t->item(row, ColSet);
    if (!setItem) return;
    bool ok = false;
    qint64 val = setItem->text().toLongLong(&ok);
    if (!ok)
    {
        btn->setText("错误");
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
        raw->setText(QString::number(pt.value));
        break;

    case ReadTimeout:
        st->setText("超时");
        st->setTextColor(Qt::red);
        raw->setText("—");
        st->setToolTip(pt.errText);
        break;

    case ReadError:
        // Modbus 异常码与错误描述显示在状态列，原始值列无有效数据
        st->setText(QString("错误 0x%1 %2")
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
    Q_UNUSED(msg);
}

int RegisterView::findRow(QTableWidget *table, int protoAddr) const
{
    int area = areaOf(table);
    if (area < 0) return -1;
    for (int i = 0; i < m_rows[area].size(); ++i)
        if (m_rows[area][i].protoAddr == protoAddr) return i;
    return -1;
}

void RegisterView::onCustomContextMenu(const QPoint &pos)
{
    QTableWidget *t = qobject_cast<QTableWidget *>(sender());
    if (!t) return;
    int area = areaOf(t);
    if (area < 0) return;
    QPoint gpos = t->viewport()->mapToGlobal(pos);

    QMenu menu(this);
    QAction *addAct = menu.addAction("快速添加...");
    QAction *delAct = menu.addAction("删除点位");
    menu.addSeparator();
    QAction *clearAct = menu.addAction("清空列表");
    clearAct->setEnabled(t->rowCount() > 0);
    QAction *chosen = menu.exec(gpos);
    if (!chosen) return;
    if (chosen == addAct)
        onQuickAddInArea(area);
    else if (chosen == delAct)
        onDeleteRowsInArea(area, t);
    else if (chosen == clearAct)
        onClearArea(area, t);
}

void RegisterView::onQuickAddInArea(int area)
{
    AddRegisterDialog dlg(area, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        int skipped = addRegisters(area, dlg.startAddr(), dlg.count());
        if (skipped > 0)
        {
            QMessageBox::information(this, "快速添加",
                QString("已跳过 %1 个与现有点位重复的地址。").arg(skipped));
        }
    }
}

void RegisterView::onDeleteRowsInArea(int area, QTableWidget *t)
{
    QList<QTableWidgetItem *> sel = t->selectedItems();
    if (sel.isEmpty()) return;
    QList<int> rows;
    foreach (QTableWidgetItem *it, sel)
        if (!rows.contains(it->row())) rows.append(it->row());
    qSort(rows.begin(), rows.end(), qGreater<int>()); // 从后往前删
    foreach (int r, rows)
    {
        t->removeRow(r);
        m_rows[area].removeAt(r);
    }
    // 复位行内控件属性（行号由垂直表头自动维护，无需手动重排）
    for (int r = 0; r < t->rowCount(); ++r)
    {
        QComboBox *c = qobject_cast<QComboBox *>(t->cellWidget(r, ColType));
        if (c) c->setProperty("row", r);
        QPushButton *b = qobject_cast<QPushButton *>(t->cellWidget(r, ColWrite));
        if (b) b->setProperty("row", r);
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
