#include "registerdata.h"

static const int kRegSpace = 65536;

RegisterData::RegisterData(QObject *parent)
    : QObject(parent)
{

    m_store.resize(4 * kRegSpace);
}

ReadPoint RegisterData::value(int areaIndex, int address) const
{
    if (areaIndex < 0 || areaIndex > 3 || address < 0 || address >= kRegSpace)
    {
        return ReadPoint();
    }
    QMutexLocker lock(&m_mutex);
    return m_store[areaIndex * kRegSpace + address];
}

bool RegisterData::contains(int areaIndex, int address) const
{
    if (areaIndex < 0 || areaIndex > 3 || address < 0 || address >= kRegSpace)
    {
        return false;
    }
    QMutexLocker lock(&m_mutex);
    return m_store[areaIndex * kRegSpace + address].status != ReadInvalid;
}

void RegisterData::update(const ReadPoint &pt)
{
    if (pt.areaIndex < 0 || pt.areaIndex > 3)
    {
        return;
    }
    if (pt.address < 0 || pt.address >= kRegSpace)
    {
        return;
    }
    {
        QMutexLocker lock(&m_mutex);
        m_store[pt.areaIndex * kRegSpace + pt.address] = pt;
    }
    emit dataChanged(pt);
}
