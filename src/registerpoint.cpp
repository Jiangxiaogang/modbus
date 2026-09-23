#include "registerpoint.h"

const AreaInfo &areaInfo(int index)
{
    static const AreaInfo areas[4] =
    {
        { 0, "DO/遥控",      1,    1,  5, 15, true  },
        { 1, "DI/遥信",  10001,    2,  0,  0, false },
        { 2, "AO/遥调",  40001,    3,  6, 16, true  },
        { 3, "AI/遥测",  30001,    4,  0,  0, false }
    };
    return areas[index];
}

QString formatAddr(int protoAddr)
{
    return QString("0x%1").arg((quint16)protoAddr, 4, 16, QLatin1Char('0'));
}
