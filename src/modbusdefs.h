#ifndef MODBUSDEFS_H
#define MODBUSDEFS_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QMetaType>

// 通道类型
enum ChannelType
{
    ChannelSerial = 0,
    ChannelNetwork,
};

// 网络类型
enum NetworkType
{
    NetworkTCP = 0,
    NetworkUDP
};

// 协议类型
enum ProtocolType
{
    ProtocolRTU  = 0,
    ProtocolTCP,
    ProtocolASCII
};

// 数据类型
enum DataType
{
    TypeBIT = 0,
    TypeU16,
    TypeS16,
    TypeU32,
    TypeS32,
    TypeF32
};
// 供跨线程 QueuedConnection 传递 DataType 参数（QObject::connect 排队参数要求已注册）
Q_DECLARE_METATYPE(DataType)

// 16位字节序：AB=高字节在前(大端，Modbus 默认)，BA=低字节在前(交换两字节)
// 32位字节序：4 字节按 A(MSB) B C D(LSB) 记，映射到两个寄存器
enum ByteOrder
{
    ByteOrderAB = 0,   // 16位: 大端
    ByteOrderBA,       // 16位: 字节交换
    ByteOrderABCD,     // 32位: Reg0=AB Reg1=CD
    ByteOrderCDAB,     // 32位: Reg0=CD Reg1=AB (字交换)
    ByteOrderBADC,     // 32位: Reg0=BA Reg1=DC (字节交换)
    ByteOrderDCBA      // 32位: Reg0=DC Reg1=BA (全交换)
};
Q_DECLARE_METATYPE(ByteOrder)

// 读取结果状态：用于区分“正常/超时/设备错误/无效”
enum ReadStatus
{
    ReadOk = 0,       // 读取成功
    ReadTimeout,      // 响应超时（无响应）
    ReadError,        // 设备返回异常响应（Modbus 异常码）
    ReadInvalid       // 其它无效（解析失败 / 功能码不符等）
};

// 一次读取结果（中转站与展示之间传递的单一对象，以引用传递）
struct ReadPoint
{
    int      areaIndex;  // 数据区 0..3
    int      address;    // 协议地址 (0基)
    int      status;     // ReadStatus
    qint64   value;      // 正常时的原始值
    qint64   errValue;   // 错误时的 Modbus 异常码
    QString  errText;    // 错误/超时描述
    ReadPoint()
        : areaIndex(0), address(0), status(ReadInvalid), value(0), errValue(0) {}
};

// 四个数据区。数组顺序即 TAB 显示顺序：DO/DI/AO/AI
// DO/遥控(线圈)      PLC 00001-09999  读01 写05/15
// DI/遥信(离散输入)   PLC 10001-19999 读02
// AO/遥调(保持寄存器) PLC 40001-49999 读03 写06/16
// AI/遥测(输入寄存器) PLC 30001-39999 读04
struct AreaInfo
{
    int      index;       // 0..3 (界面顺序)
    QString  name;        // 区名
    int      plcBase;     // PLC 起始编号 (00001->1, 10001... )
    int      readFunc;    // 读功能码
    int      writeFuncSingle; // 单写功能码 (05/06)
    int      writeFuncMulti;  // 多写功能码 (15/16)
    bool     writable;    // 是否可写 (0区/4区)
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

// 单条寄存器读取计划项
struct RegPlanItem
{
    int       address;    // 协议地址 (0基)
    DataType  type;
    ByteOrder byteOrder;  // 仅 AI/AO 有效；位区恒为 ByteOrderAB
};

// 连接 + 协议 配置
struct ModbusConfig
{
    ChannelType  channel;
    ProtocolType protocol;

    // 串口
    QString portName;
    int     baudRate;
    int     dataBits;
    int     stopBits;
    int     parity;      // 0=无 1=奇 2=偶

    // 网络
    NetworkType netType;
    QString netAddr;
    int     netPort;

    // 协议
    int     slave;            // 从站地址
    int     responseTimeout;  // 响应超时 ms
    int     pollInterval;     // 轮询间隔 ms
    int     readMode;         // 读取模式 0=单点模式/1=批量模式
    int     coilWriteFunc;    // 遥控功能码 5 / 15
    int     regWriteFunc;     // 遥调功能码 6 / 16
};
// 供跨线程 QueuedConnection 值传递连接配置（QObject::connect 排队参数要求已注册）
Q_DECLARE_METATYPE(ModbusConfig)

// CRC16 (Modbus)
quint16 modbusCrc(const char *data, int len);
// LRC (ASCII)
quint8  modbusLrc(const char *data, int len);

// 交换 16 位数据的两个字节（AB <-> BA 互为逆运算）
inline quint16 swapBytes16(quint16 v)
{
    return (quint16)((v << 8) | (v >> 8));
}

// 是否为占用两个连续寄存器的 32 位类型
inline bool is32BitType(DataType t)
{
    return t == TypeU32 || t == TypeS32 || t == TypeF32;
}

// 类型占用的寄存器个数
inline int regCountOf(DataType t)
{
    return is32BitType(t) ? 2 : 1;
}

// 字节序是否适用于该数据类型
inline bool byteOrderValidFor(DataType t, ByteOrder bo)
{
    if (is32BitType(t))
        return bo == ByteOrderABCD || bo == ByteOrderCDAB
            || bo == ByteOrderBADC || bo == ByteOrderDCBA;
    return bo == ByteOrderAB || bo == ByteOrderBA;
}

// 32位还原：由两个寄存器(低地址在前)按字节序合成逻辑值
inline quint32 decode32(quint16 w0, quint16 w1, ByteOrder bo)
{
    switch (bo)
    {
    case ByteOrderCDAB: return ((quint32)w1 << 16) | w0;
    case ByteOrderBADC: return ((quint32)swapBytes16(w0) << 16) | swapBytes16(w1);
    case ByteOrderDCBA: return ((quint32)swapBytes16(w1) << 16) | swapBytes16(w0);
    default:            return ((quint32)w0 << 16) | w1; // ABCD
    }
}

// 32位编码：由逻辑值按字节序拆成两个寄存器(低地址在前)，与 decode32 互逆
inline void encode32(quint32 v, ByteOrder bo, quint16 &w0, quint16 &w1)
{
    const quint16 hi = (quint16)(v >> 16);
    const quint16 lo = (quint16)(v & 0xFFFF);
    switch (bo)
    {
    case ByteOrderCDAB: w0 = lo;                w1 = hi;                break;
    case ByteOrderBADC: w0 = swapBytes16(hi);   w1 = swapBytes16(lo);   break;
    case ByteOrderDCBA: w0 = swapBytes16(lo);   w1 = swapBytes16(hi);   break;
    default:            w0 = hi;                w1 = lo;                break; // ABCD
    }
}

// 数据类型全局定义表（界面文本的唯一来源，顺序即下拉显示顺序）
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

// 全部可编辑数据类型（不含位类型）的文本列表
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

// 按位宽过滤：16位返回 INT16/INT16U，32位返回 INT32/INT32U/FLOAT32
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

// 字节序全局定义表（界面文本的唯一来源）
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

// 按数据类型位宽返回可选字节序文本：16位 AB/BA，32位 ABCD/CDAB/BADC/DCBA
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

// 把协议地址格式化为 0xXXXX
QString formatAddr(int protoAddr);

#endif // MODBUSDEFS_H
