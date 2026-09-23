#include "registertypes.h"

static const DataTypeDef kDataTypeTable[] =
{
    { TypeBIT,  "BIT"     },
    { TypeS16,  "INT16"   },
    { TypeU16,  "UINT16"  },
    { TypeS32,  "INT32"   },
    { TypeU32,  "UINT32"  },
    { TypeF32,  "FLOAT32" }
};

const DataTypeDef *dataTypeTable(int *count)
{
    if (count)
    {
        *count = (int)(sizeof(kDataTypeTable) / sizeof(kDataTypeTable[0]));
    }
    return kDataTypeTable;
}

QString dataTypeText(DataType t)
{
    int n = 0;
    const DataTypeDef *tab = dataTypeTable(&n);
    for (int i = 0; i < n; ++i)
    {
        if (tab[i].type == t)
        {
            return QString::fromLatin1(tab[i].text);
        }
    }
    return QString::fromLatin1(tab[0].text);
}

DataType dataTypeFromText(const QString &t)
{
    int n = 0;
    const DataTypeDef *tab = dataTypeTable(&n);
    for (int i = 0; i < n; ++i)
    {
        if (t == QLatin1String(tab[i].text))
        {
            return tab[i].type;
        }
    }
    return TypeU16;
}

QStringList dataTypeTextsAll()
{
    int n = 0;
    const DataTypeDef *tab = dataTypeTable(&n);
    QStringList list;
    for (int i = 0; i < n; ++i)
    {
        if (tab[i].type != TypeBIT)
        {
            list << QString::fromLatin1(tab[i].text);
        }
    }
    return list;
}

QStringList dataTypeTextsFor(DataType t)
{
    if (t == TypeBIT)
    {
        return QStringList{ dataTypeText(TypeBIT) };
    }
    const bool wide = is32BitType(t);
    int n = 0;
    const DataTypeDef *tab = dataTypeTable(&n);
    QStringList list;
    for (int i = 0; i < n; ++i)
    {
        if (tab[i].type != TypeBIT && is32BitType(tab[i].type) == wide)
        {
            list << QString::fromLatin1(tab[i].text);
        }
    }
    return list;
}

static const ByteOrderDef kByteOrderTable[] =
{
    { ByteOrderAB,   "AB"   },
    { ByteOrderBA,   "BA"   },
    { ByteOrderABCD, "ABCD" },
    { ByteOrderCDAB, "CDAB" },
    { ByteOrderBADC, "BADC" },
    { ByteOrderDCBA, "DCBA" }
};

const ByteOrderDef *byteOrderTable(int *count)
{
    if (count)
    {
        *count = (int)(sizeof(kByteOrderTable) / sizeof(kByteOrderTable[0]));
    }
    return kByteOrderTable;
}

QString byteOrderText(ByteOrder bo)
{
    int n = 0;
    const ByteOrderDef *tab = byteOrderTable(&n);
    for (int i = 0; i < n; ++i)
    {
        if (tab[i].order == bo)
        {
            return QString::fromLatin1(tab[i].text);
        }
    }
    return QString::fromLatin1(tab[0].text);
}

ByteOrder byteOrderFromText(const QString &t)
{
    int n = 0;
    const ByteOrderDef *tab = byteOrderTable(&n);
    for (int i = 0; i < n; ++i)
    {
        if (t == QLatin1String(tab[i].text))
        {
            return tab[i].order;
        }
    }
    return ByteOrderAB;
}

QStringList byteOrderTextsFor(DataType t)
{
    int n = 0;
    const ByteOrderDef *tab = byteOrderTable(&n);
    QStringList list;
    for (int i = 0; i < n; ++i)
    {
        if (byteOrderValidFor(t, tab[i].order))
        {
            list << QString::fromLatin1(tab[i].text);
        }
    }
    return list;
}
