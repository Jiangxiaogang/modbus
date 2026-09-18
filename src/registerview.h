#ifndef REGISTERVIEW_H
#define REGISTERVIEW_H

#include "modbusdefs.h"
#include <QWidget>
#include <QList>
#include <QStyledItemDelegate>

class QTabWidget;
class QTableWidget;
class QTableWidgetItem;
class QStyleOptionViewItem;
class QModelIndex;
class RegisterView;
class RealtimeData;

// 类型列编辑器委托：仅在进入编辑(双击)时临时创建下拉框，
// 静止时不驻留控件——避免下拉框吃掉滚轮事件，也减少批量点位的内存与创建开销
class TypeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    TypeDelegate(int area, RegisterView *owner, QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;

private slots:
    void commitAndCloseEditor();

private:
    int           m_area;
    RegisterView *m_owner;
};

struct RowData
{
    QString  name;
    int      plcAddr;     // PLC 地址 (如 40001)
    int      protoAddr;   // 协议地址 (0基)
    DataType type;
    // 实时值 (valid / rawValue) 由 RealtimeData 中转站统一持有，
    // 本结构仅保留点位定义，避免双份数据源。
};

// 寄存器数据区：4 个 TAB（0/1/3/4 区）
class RegisterView : public QWidget
{
    Q_OBJECT
public:
    explicit RegisterView(QWidget *parent = 0);

    // 类型编辑提交回调（由 TypeDelegate 调用）：更新点位类型并同步读取计划
    void commitType(int area, int row, DataType tp);

    // 绑定实时数据中转站，供切换数值格式时重新渲染原始值
    void setRealtimeData(RealtimeData *data);

signals:
    // 某区读取计划变化（增删/改类型）
    void planChanged(int areaIndex, QList<RegPlanItem> *items);
    // 请求写入某点
    void writeRequested(int areaIndex, int protoAddr, DataType type, qint64 value);

public slots:
    void onReadResult(const ReadPoint &pt);
    void onWriteResult(int areaIndex, int protoAddr, bool ok, const QString &msg);

private slots:
    void onCustomContextMenu(const QPoint &pos);
    void onNameChanged(QTableWidgetItem *item);
    void onItemDoubleClicked(QTableWidgetItem *item);
    void onWriteClicked();
    void onTabChanged(int index);

private:
    void setupTab(int areaIndex);
    int  addRegisters(int areaIndex, int startAddr, int count); // 返回因重复而跳过的数量
    void fillRow(QTableWidget *t, int areaIndex, int r,
                 const RowData &rd, const AreaInfo &info);
    void rebuildPlan(int areaIndex);
    int  areaOf(QTableWidget *table) const;
    int  findRow(QTableWidget *table, int protoAddr) const;
    void onQuickAddInArea(int area);
    void onDeleteRowsInArea(int area, QTableWidget *t);
    void onClearArea(int area, QTableWidget *t);
    QString addrText(int protoAddr) const;  // 按当前格式显示协议地址
    void setAddrHex(bool hex);              // 切换地址格式并刷新所有区
    QString valueText(qint64 value) const;  // 按当前格式显示数值
    void setValueHex(bool hex);             // 切换数值格式并刷新所有区原始值

    // 序号由垂直表头（Qt 自带行号）提供，故不再单独建列
    // 列序须与 setupTab 中表头顺序一致
    enum Col { ColName = 0, ColAddr, ColType, ColRaw,
               ColSet, ColWrite, ColStatus
             };

    QTabWidget        *m_tabs;
    QTableWidget      *m_tables[4];
    QList<RowData>     m_rows[4];
    bool               m_hexAddr;  // 地址列格式：true=16进制 false=10进制
    bool               m_hexValue; // 原始值格式：true=16进制 false=10进制
    RealtimeData      *m_data;     // 实时数据中转站，切换数值格式时读取最新值
};

#endif // REGISTERVIEW_H
