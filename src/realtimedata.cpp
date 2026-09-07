#include "realtimedata.h"

// 每个数据区的协议地址空间 (0..65535)
static const int kRegSpace = 65536;

RealtimeData::RealtimeData(QObject *parent)
    : QObject(parent)
{
    // 构造时一次性申请 4 个区的全部寄存器空间，避免运行时动态增长
    m_store.resize(4 * kRegSpace);
}

ReadPoint RealtimeData::value(int areaIndex, int address) const
{
    if (areaIndex < 0 || areaIndex > 3 || address < 0 || address >= kRegSpace)
        return ReadPoint();
    QMutexLocker lock(&m_mutex);
    return m_store[areaIndex * kRegSpace + address];
}

bool RealtimeData::contains(int areaIndex, int address) const
{
    if (areaIndex < 0 || areaIndex > 3 || address < 0 || address >= kRegSpace)
        return false;
    QMutexLocker lock(&m_mutex);
    return m_store[areaIndex * kRegSpace + address].status != ReadInvalid;
}

void RealtimeData::update(const ReadPoint &pt)
{
    if (pt.areaIndex < 0 || pt.areaIndex > 3) return;
    if (pt.address < 0 || pt.address >= kRegSpace) return;
    {
        QMutexLocker lock(&m_mutex);
        m_store[pt.areaIndex * kRegSpace + pt.address] = pt;
    }
    emit dataChanged(pt);
}
