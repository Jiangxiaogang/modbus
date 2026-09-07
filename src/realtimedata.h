#ifndef REALTIMEDATA_H
#define REALTIMEDATA_H

#include "modbusdefs.h"
#include <QObject>
#include <QVector>
#include <QMutex>

// 实时数据对象（中转站）
// - 读到数据：调用 update() 写入本对象（API 方式，非 slot）
// - 展示数据：RegisterView 经由本对象的 dataChanged 信号获取最新值
// 本对象是实时数据的唯一权威来源，解耦采集线程与 UI 展示。
class RealtimeData : public QObject
{
    Q_OBJECT
public:
    explicit RealtimeData(QObject *parent = 0);

    // 查询某点最新值（供展示侧读取）
    ReadPoint value(int areaIndex, int address) const;
    bool      contains(int areaIndex, int address) const;

    // API：写入工作线程读到的数据（单一对象，非 slot 接收）
    void update(const ReadPoint &pt);

signals:
    // 数据变化后转发给展示侧（单一对象引用参数）
    void dataChanged(const ReadPoint &pt);

private:
    QVector<ReadPoint> m_store;  // 4区 × 全部寄存器空间，索引 = area*空间 + addr
    mutable QMutex     m_mutex;
};

#endif // REALTIMEDATA_H
