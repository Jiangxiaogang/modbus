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
class QPushButton;
class RegisterView;
class RegisterData;

class TypeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    TypeDelegate(int area, RegisterView *owner, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

private:
    int           m_area;
    RegisterView *m_owner;
};

class ByteOrderDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ByteOrderDelegate(int area, RegisterView *owner, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

private:
    int           m_area;
    RegisterView *m_owner;
};

struct RowData
{
    QString   name;
    int       plcAddr;
    int       protoAddr;
    DataType  type;
    ByteOrder byteOrder;

};

class RegisterView : public QWidget
{
    Q_OBJECT
public:
    explicit RegisterView(QWidget *parent = nullptr);

    void commitType(int area, int row, DataType tp);

    void commitByteOrder(int area, int row, ByteOrder bo);

    DataType rowType(int area, int row) const;

    void setRegisterData(RegisterData *data);

signals:

    void planChanged(int areaIndex, QList<RegPlanItem> *items);

    void writeRequested(int areaIndex, int protoAddr, DataType type,
                        ByteOrder byteOrder, qint64 value);

public slots:
    void onReadResult(const ReadPoint &pt);
    void onWriteResult(int areaIndex, int protoAddr, bool ok, const QString &msg);

private slots:
    void onNameChanged(QTableWidgetItem *item);
    void onItemDoubleClicked(QTableWidgetItem *item);

private:
    void setupTab(int areaIndex);
    int  addRegisters(int areaIndex, int startAddr, int count,
                      DataType type, ByteOrder byteOrder);
    void fillRow(QTableWidget *t, int areaIndex, int r,
                 const RowData &rd, const AreaInfo &info);
    void rebuildPlan(int areaIndex);
    int  areaOf(QTableWidget *table) const;
    int  findRow(QTableWidget *table, int protoAddr) const;
    void onAddPointsInArea(int area);
    void onDeleteRowsInArea(int area, QTableWidget *t);
    void onClearArea(int area, QTableWidget *t);
    void showAreaMenu(QTableWidget *t, int area, const QPoint &pos);
    void doWrite(int area, QTableWidget *t, QPushButton *btn);
    QString addrText(int protoAddr) const;
    void setAddrHex(bool hex);
    QString valueText(DataType type, qint64 value) const;
    void setValueHex(bool hex);

    enum Col { ColName = 0, ColAddr, ColType, ColByteOrder, ColRaw,
               ColSet, ColWrite, ColStatus
             };

    QTabWidget        *m_tabs;
    QTableWidget      *m_tables[4];
    QList<RowData>     m_rows[4];
    bool               m_hexAddr;
    bool               m_hexValue;
    RegisterData      *m_data = nullptr;
};

#endif
