#ifndef REGISTERVIEW_H
#define REGISTERVIEW_H

#include "modbusdefs.h"
#include <QWidget>
#include <QList>

class QTabWidget;
class QTableWidget;
class QTableWidgetItem;

struct RowData {
    QString  name;
    int      plcAddr;     // PLC 地址 (如 40001)
    int      protoAddr;   // 协议地址 (0基)
    bool     valid;
    DataType type;
    qint64   rawValue;
};

// 寄存器数据区：4 个 TAB（0/1/3/4 区）
class RegisterView : public QWidget
{
    Q_OBJECT
public:
    explicit RegisterView(QWidget* parent = 0);

signals:
    // 某区读取计划变化（增删/改类型）
    void planChanged(int areaIndex, const QList<RegPlanItem>& items);
    // 请求写入某点
    void writeRequested(int areaIndex, int protoAddr, DataType type, qint64 value);

public slots:
    void onReadResult(int areaIndex, int protoAddr, bool valid, qint64 value);
    void onWriteResult(int areaIndex, int protoAddr, bool ok, const QString& msg);

private slots:
    void onCustomContextMenu(const QPoint& pos);
    void onTypeChanged(int index);
    void onNameChanged(QTableWidgetItem* item);
    void onWriteClicked();
    void onTabChanged(int index);

private:
    void setupTab(int areaIndex);
    void addRegisters(int areaIndex, int startPlc, int count);
    void rebuildPlan(int areaIndex);
    int  areaOf(QTableWidget* table) const;
    int  findRow(QTableWidget* table, int protoAddr) const;
    void onQuickAddInArea(int area);
    void onDeleteRowsInArea(int area, QTableWidget* t);

    enum Col { ColNo=0, ColName, ColAddr, ColStatus, ColType,
               ColRaw, ColSet, ColWrite };

    QTabWidget*        m_tabs;
    QTableWidget*      m_tables[4];
    QList<RowData>     m_rows[4];
};

#endif // REGISTERVIEW_H
