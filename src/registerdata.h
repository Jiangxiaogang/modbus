#ifndef REGISTERDATA_H
#define REGISTERDATA_H

#include "modbusdefs.h"
#include <QObject>
#include <QVector>
#include <QMutex>

class RegisterData : public QObject
{
    Q_OBJECT
public:
    explicit RegisterData(QObject *parent = 0);

    ReadPoint value(int areaIndex, int address) const;
    bool      contains(int areaIndex, int address) const;

    void update(const ReadPoint &pt);

signals:

    void dataChanged(const ReadPoint &pt);

private:
    QVector<ReadPoint> m_store;
    mutable QMutex     m_mutex;
};

#endif
