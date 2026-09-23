#ifndef MODBUSCONTROLLER_H
#define MODBUSCONTROLLER_H

#include "registerpoint.h"
#include <QList>

class IModbusController
{
public:
    virtual ~IModbusController() = default;

    virtual void connectDevice() = 0;
    virtual void disconnectDevice() = 0;
    virtual void applyConfig() = 0;

    virtual void setAreaPlan(int areaIndex, const QList<RegPlanItem> &items) = 0;
    virtual void writeRegister(int areaIndex, int address, DataType type,
                               ByteOrder byteOrder, qint64 value) = 0;
};

#endif
