#include "registerview.h"
#include "modbusdefs.h"
#include "addregisterdialog.h"

#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <QHeaderView>
#include <QVBoxLayout>

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
    headers << "寄存器名称" << "寄存器地址" << "数据状态" << "数据类型" << "原始值" << "设定值" << "写入";
    t->setHorizontalHeaderLabels(headers);
    t->horizontalHeader()->setStretchLastSection(false);
    t->setColumnWidth(ColName, 120);
    t->setColumnWidth(ColAddr, 90);
    t->setColumnWidth(ColStatus, 70);
    t->setColumnWidth(ColType, 70);
    t->setColumnWidth(ColRaw, 80);
    t->setColumnWidth(ColSet, 80);
    t->setColumnWidth(ColWrite, 60);
    t->horizontalHeader()->setHighlightSections(false);
    t->verticalHeader()->setHighlightSections(false);
    int hdrH = t->horizontalHeader()->height();
    if (hdrH > 0)
    {
         t->verticalHeader()->setDefaultSectionSize(hdrH);
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

void RegisterView::addRegisters(int areaIndex, int startPlc, int count)
{
    QTableWidget *t = m_tables[areaIndex];
    const AreaInfo &info = areaInfo(areaIndex);
    DataType def = (info.readFunc == 1 || info.readFunc == 2) ? TypeBIT : TypeU16;

    int base = info.plcBase;
    t->setSortingEnabled(false);
    for (int i = 0; i < count; ++i)
    {
        int plc = startPlc + i;
        if (plc < base || plc > base + 9998)
            continue;
        int proto = plc - base;

        RowData rd;
        // 0区(遥控) PLC 地址为 00001-09999，名称补前导 0 至 5 位保持对齐
        rd.name = (areaIndex == 0)
                  ? QString::number(plc).rightJustified(5, '0')
                  : QString::number(plc);
        rd.plcAddr = plc;
        rd.protoAddr = proto;
        rd.valid = false;
        rd.type = def;
        rd.rawValue = 0;
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

        QComboBox *typeCombo = new QComboBox(t);
        typeCombo->addItems(QStringList() << "BIT" << "U16" << "S16");
        typeCombo->setCurrentIndex((int)def);
        typeCombo->setProperty("row", r);
        typeCombo->setStyleSheet("border: none;");
        t->setCellWidget(r, ColType, typeCombo);
        connect(typeCombo, SIGNAL(currentIndexChanged(int)),
                this, SLOT(onTypeChanged(int)));

        QLineEdit *setEdit = new QLineEdit(t);
        setEdit->setProperty("row", r);
        // 去掉边框与背景，使设定值输入框融入表格样式
        setEdit->setStyleSheet("border: none;");
        t->setCellWidget(r, ColSet, setEdit);

        QPushButton *wbtn = new QPushButton("写入", t);
        wbtn->setProperty("row", r);
       // wbtn->setStyleSheet("border: none;");//"border: none; background: transparent;");
        t->setCellWidget(r, ColWrite, wbtn);
        connect(wbtn, SIGNAL(clicked()), this, SLOT(onWriteClicked()));

        // 不可写区域：仍显示设定值与写入按钮，但置为禁用状态
        if (!info.writable)
        {
            setEdit->setEnabled(false);
            wbtn->setEnabled(false);
        }
    }
    t->setSortingEnabled(false);
    rebuildPlan(areaIndex);
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
    QTableWidget *t = qobject_cast<QTableWidget *>(combo->parent());
    if (!t) return;
    int area = areaOf(t);
    int row = combo->property("row").toInt();
    if (area < 0 || row < 0 || row >= m_rows[area].size()) return;
    m_rows[area][row].type = (DataType)combo->currentIndex();
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
    QTableWidget *t = qobject_cast<QTableWidget *>(btn->parent());
    if (!t) return;
    int area = areaOf(t);
    int row = btn->property("row").toInt();
    if (area < 0 || row < 0 || row >= m_rows[area].size()) return;

    RowData &rd = m_rows[area][row];
    QLineEdit *setEdit = qobject_cast<QLineEdit *>(t->cellWidget(row, ColSet));
    if (!setEdit) return;
    bool ok = false;
    qint64 val = setEdit->text().toLongLong(&ok);
    if (!ok)
    {
        btn->setText("错误");
        return;
    }
    btn->setText("写入");
    emit writeRequested(area, rd.protoAddr, rd.type, val);
}

void RegisterView::onReadResult(int areaIndex, int protoAddr, bool valid, qint64 value)
{
    if (areaIndex < 0 || areaIndex > 3) return;
    QTableWidget *t = m_tables[areaIndex];
    int row = findRow(t, protoAddr);
    if (row < 0) return;
    RowData &rd = m_rows[areaIndex][row];
    rd.valid = valid;
    rd.rawValue = value;

    QTableWidgetItem *st = t->item(row, ColStatus);
    QTableWidgetItem *raw = t->item(row, ColRaw);
    if (valid)
    {
        st->setText("有效");
        st->setTextColor(Qt::darkGreen);
        raw->setText(QString::number(value));
    }
    else
    {
        st->setText("无效");
        st->setTextColor(Qt::red);
        raw->setText("");
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
    QAction *chosen = menu.exec(gpos);
    if (!chosen) return;
    if (chosen == addAct)
        onQuickAddInArea(area);
    else if (chosen == delAct)
        onDeleteRowsInArea(area, t);
}

void RegisterView::onQuickAddInArea(int area)
{
    AddRegisterDialog dlg(area, this);
    if (dlg.exec() == QDialog::Accepted)
        addRegisters(area, dlg.startPlc(), dlg.count());
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
        QLineEdit *e = qobject_cast<QLineEdit *>(t->cellWidget(r, ColSet));
        if (e) e->setProperty("row", r);
        QPushButton *b = qobject_cast<QPushButton *>(t->cellWidget(r, ColWrite));
        if (b) b->setProperty("row", r);
    }
    rebuildPlan(area);
}
