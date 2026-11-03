#include "registerpoint.h"

const AreaInfo &areaInfo(int index)
{
    static const AreaInfo areas[4] =
    {
        { 0, "遥控(DO)",      1,    1,  5, 15, true  },
        { 1, "遥信(DI)",  10001,    2,  0,  0, false },
        { 2, "遥调(AO)",  40001,    3,  6, 16, true  },
        { 3, "遥测(AI)",  30001,    4,  0,  0, false }
    };
    return areas[index];
}

QString formatAddr(int protoAddr)
{
    return QString("0x%1").arg((quint16)protoAddr, 4, 16, QLatin1Char('0'));
}
