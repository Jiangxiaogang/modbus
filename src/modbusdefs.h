#ifndef MODBUSDEFS_H
#define MODBUSDEFS_H

#include <QString>
#include <QStringList>
#include <QList>

enum ChannelType
{
    ChannelSerial = 0,
    ChannelNetwork,
};

enum NetworkType
{
    NetworkTCP = 0,
    NetworkUDP
};

enum ProtocolType
{
    ProtocolRTU  = 0,
    ProtocolTCP,
    ProtocolASCII
};

enum DataType
{
    TypeBIT = 0,
    TypeU16,
    TypeS16,
    TypeU32,
    TypeS32,
    TypeF32
};

enum ByteOrder
{
    ByteOrderAB = 0,
    ByteOrderBA,
    ByteOrderABCD,
    ByteOrderCDAB,
    ByteOrderBADC,
    ByteOrderDCBA
};

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

inline const AreaInfo &areaInfo(int index)
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

struct RegPlanItem
{
    int       address;
    DataType  type;
    ByteOrder byteOrder;
};

struct ModbusConfig
{
    ChannelType  channel;
    ProtocolType protocol;

    QString portName;
    int     baudRate;
    int     dataBits;
    int     stopBits;
    int     parity;

    NetworkType netType;
    QString netAddr;
    int     netPort;

    int     slave;
    int     responseTimeout;
    int     pollInterval;
    int     readMode;
    int     coilWriteFunc;
    int     regWriteFunc;
};

quint16 modbusCrc(const char *data, int len);

quint8  modbusLrc(const char *data, int len);

inline quint16 swapBytes16(quint16 v)
{
    return (quint16)((v << 8) | (v >> 8));
}

inline bool is32BitType(DataType t)
{
    return t == TypeU32 || t == TypeS32 || t == TypeF32;
}

inline int regCountOf(DataType t)
{
    return is32BitType(t) ? 2 : 1;
}

inline bool byteOrderValidFor(DataType t, ByteOrder bo)
{
    if (is32BitType(t))
        return bo == ByteOrderABCD || bo == ByteOrderCDAB
            || bo == ByteOrderBADC || bo == ByteOrderDCBA;
    return bo == ByteOrderAB || bo == ByteOrderBA;
}

inline quint32 decode32(quint16 w0, quint16 w1, ByteOrder bo)
{
    switch (bo)
    {
    case ByteOrderCDAB: return ((quint32)w1 << 16) | w0;
    case ByteOrderBADC: return ((quint32)swapBytes16(w0) << 16) | swapBytes16(w1);
    case ByteOrderDCBA: return ((quint32)swapBytes16(w1) << 16) | swapBytes16(w0);
    default:            return ((quint32)w0 << 16) | w1;
    }
}

inline void encode32(quint32 v, ByteOrder bo, quint16 &w0, quint16 &w1)
{
    const quint16 hi = (quint16)(v >> 16);
    const quint16 lo = (quint16)(v & 0xFFFF);
    switch (bo)
    {
    case ByteOrderCDAB: w0 = lo;                w1 = hi;                break;
    case ByteOrderBADC: w0 = swapBytes16(hi);   w1 = swapBytes16(lo);   break;
    case ByteOrderDCBA: w0 = swapBytes16(lo);   w1 = swapBytes16(hi);   break;
    default:            w0 = hi;                w1 = lo;                break;
    }
}

struct DataTypeDef
{
    DataType    type;
    const char *text;
};
inline const DataTypeDef *dataTypeTable(int *count)
{
    static const DataTypeDef table[] =
    {
        { TypeBIT,  "BIT"     },
        { TypeS16,  "INT16"   },
        { TypeU16,  "UINT16"  },
        { TypeS32,  "INT32"   },
        { TypeU32,  "UINT32"  },
        { TypeF32,  "FLOAT32" }
    };
    if (count) *count = (int)(sizeof(table) / sizeof(table[0]));
    return table;
}

inline QString dataTypeText(DataType t)
{
    int n = 0;
    const DataTypeDef *tab = dataTypeTable(&n);
    for (int i = 0; i < n; ++i)
        if (tab[i].type == t)
            return QString::fromLatin1(tab[i].text);
    return QString::fromLatin1(tab[0].text);
}

inline DataType dataTypeFromText(const QString &t)
{
    int n = 0;
    const DataTypeDef *tab = dataTypeTable(&n);
    for (int i = 0; i < n; ++i)
        if (t == QLatin1String(tab[i].text))
            return tab[i].type;
    return TypeU16;
}

inline QStringList dataTypeTextsAll()
{
    int n = 0;
    const DataTypeDef *tab = dataTypeTable(&n);
    QStringList list;
    for (int i = 0; i < n; ++i)
        if (tab[i].type != TypeBIT)
            list << QString::fromLatin1(tab[i].text);
    return list;
}

inline QStringList dataTypeTextsFor(DataType t)
{
    if (t == TypeBIT)
        return QStringList{ dataTypeText(TypeBIT) };
    const bool wide = is32BitType(t);
    int n = 0;
    const DataTypeDef *tab = dataTypeTable(&n);
    QStringList list;
    for (int i = 0; i < n; ++i)
        if (tab[i].type != TypeBIT && is32BitType(tab[i].type) == wide)
            list << QString::fromLatin1(tab[i].text);
    return list;
}

struct ByteOrderDef
{
    ByteOrder   order;
    const char *text;
};
inline const ByteOrderDef *byteOrderTable(int *count)
{
    static const ByteOrderDef table[] =
    {
        { ByteOrderAB,   "AB"   },
        { ByteOrderBA,   "BA"   },
        { ByteOrderABCD, "ABCD" },
        { ByteOrderCDAB, "CDAB" },
        { ByteOrderBADC, "BADC" },
        { ByteOrderDCBA, "DCBA" }
    };
    if (count) *count = (int)(sizeof(table) / sizeof(table[0]));
    return table;
}

inline QString byteOrderText(ByteOrder bo)
{
    int n = 0;
    const ByteOrderDef *tab = byteOrderTable(&n);
    for (int i = 0; i < n; ++i)
        if (tab[i].order == bo)
            return QString::fromLatin1(tab[i].text);
    return QString::fromLatin1(tab[0].text);
}

inline ByteOrder byteOrderFromText(const QString &t)
{
    int n = 0;
    const ByteOrderDef *tab = byteOrderTable(&n);
    for (int i = 0; i < n; ++i)
        if (t == QLatin1String(tab[i].text))
            return tab[i].order;
    return ByteOrderAB;
}

inline QStringList byteOrderTextsFor(DataType t)
{
    int n = 0;
    const ByteOrderDef *tab = byteOrderTable(&n);
    QStringList list;
    for (int i = 0; i < n; ++i)
        if (byteOrderValidFor(t, tab[i].order))
            list << QString::fromLatin1(tab[i].text);
    return list;
}

QString formatAddr(int protoAddr);

#endif
