#ifndef REGISTERTYPES_H
#define REGISTERTYPES_H

#include <QString>
#include <QStringList>

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
    {
        return bo == ByteOrderABCD || bo == ByteOrderCDAB
            || bo == ByteOrderBADC || bo == ByteOrderDCBA;
    }
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

const DataTypeDef *dataTypeTable(int *count);

QString dataTypeText(DataType t);

DataType dataTypeFromText(const QString &t);

QStringList dataTypeTextsAll();

QStringList dataTypeTextsFor(DataType t);

struct ByteOrderDef
{
    ByteOrder   order;
    const char *text;
};

const ByteOrderDef *byteOrderTable(int *count);

QString byteOrderText(ByteOrder bo);

ByteOrder byteOrderFromText(const QString &t);

QStringList byteOrderTextsFor(DataType t);

#endif
