#ifndef REGISTERPOINT_H
#define REGISTERPOINT_H

#include "registertypes.h"
#include <QString>

enum ReadStatus
{
    ReadOk = 0,
    ReadTimeout,
    ReadError,
    ReadInvalid
};

struct ReadPoint
{
    int      areaIndex;
    int      address;
    int      status;
    qint64   value;
    qint64   errValue;
    QString  errText;
    ReadPoint()
        : areaIndex(0), address(0), status(ReadInvalid), value(0), errValue(0) {}
};

struct AreaInfo
{
    int      index;
    QString  name;
    int      plcBase;
    int      readFunc;
    int      writeFuncSingle;
    int      writeFuncMulti;
    bool     writable;
};

const AreaInfo &areaInfo(int index);

struct RegPlanItem
{
    int       address;
    DataType  type;
    ByteOrder byteOrder;
};

QString formatAddr(int protoAddr);

#endif
