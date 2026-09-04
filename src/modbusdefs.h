#ifndef MODBUSDEFS_H
#define MODBUSDEFS_H

#include <QString>
#include <QList>

// 通道类型
enum ChannelType
{
    ChannelSerial = 0,
    ChannelTcp,
    ChannelUdp
};

// 协议类型
enum ProtocolType
{
    ProtocolRTU  = 0,   // ModbusRTU
    ProtocolTCP,        // ModbusTCP
    ProtocolASCII       // ModbusASCII
};

// 数据类型
enum DataType
{
    TypeBIT = 0,
    TypeU16,
    TypeS16
};

// 四个数据区。index 用于数组下标 0..3
// 0区: 遥控(线圈)    PLC 00001-09999  读01 写05/15
// 1区: 遥信(离散输入) PLC 10001-19999 读02
// 3区: 遥调(输入寄存器) PLC 30001-39999 读04
// 4区: 遥测(保持寄存器) PLC 40001-49999 读03 写06/16
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
        { 0, "0区(遥控)",      1,    1,  5, 15, true  },
        { 1, "1区(遥信)",  10001,    2,  0,  0, false },
        { 2, "3区(遥调)",  30001,    4,  0,  0, false },
        { 3, "4区(遥测)",  40001,    3,  6, 16, true  }
    };
    return areas[index];
}

// 单条寄存器读取计划项
struct RegPlanItem
{
    int      address;    // 协议地址 (0基)
    DataType type;
};

// 连接 + 协议 配置
struct ModbusConfig
{
    ChannelType  channel;
    ProtocolType protocol;

    // 串口
    QString serPortName;
    int     baudRate;
    int     dataBits;
    int     stopBits;
    int     parity;      // 'N' 'E' 'O'

    // 网络
    QString netAddr;
    int     netPort;

    // 协议
    int     slave;            // 从站地址
    int     responseTimeout;  // 响应超时 ms
    int     pollInterval;     // 轮询间隔 ms
    int     readQuantity;     // 单次读取数量
    int     coilWriteFunc;    // 遥控功能码 5 / 15
    int     regWriteFunc;     // 遥调功能码 6 / 16
};

// CRC16 (Modbus)
quint16 modbusCrc(const char *data, int len);
// LRC (ASCII)
quint8  modbusLrc(const char *data, int len);

// 把协议地址格式化为 0xXXXX
QString formatAddr(int protoAddr);

#endif // MODBUSDEFS_H
