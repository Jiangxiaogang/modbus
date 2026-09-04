#ifndef MODBUSCODEC_H
#define MODBUSCODEC_H

#include <QByteArray>
#include "modbusdefs.h"

// Modbus 协议编解码：负责在 PDU 与"传输帧"之间转换
//  - RTU : [从站][功能码][数据...][CRCLo][CRCHi]
//  - ASCII: ':' + Hex(从站,功能码,数据...,LRC) + CR LF
//  - TCP : [事务2][协议2=0][长度2][单元1][功能码][数据...] (MBAP)
class ModbusCodec
{
public:
    // 由从站/功能码/PDU 编码出完整传输帧
    static QByteArray encode(ProtocolType proto, quint8 slave,
                             quint8 func, const QByteArray& pdu);

    // 从收到的字节流中提取一帧完整数据；不完整返回空，frameLen 返回已消费字节数
    static QByteArray tryExtract(ProtocolType proto,
                                 const QByteArray& buffer, int* frameLen);

    // 解析一帧，得到从站/功能码/PDU；并校验 CRC/LRC/MBAP 长度
    static bool decode(ProtocolType proto, const QByteArray& frame,
                       quint8& slave, quint8& func, QByteArray& pdu);

    // 异常功能码检测：func == reqFunc | 0x80
    static bool isException(quint8 respFunc, quint8 reqFunc)
    { return (respFunc & 0x7F) == (reqFunc & 0x7F) && (respFunc & 0x80); }

    static QString exceptionText(quint8 code);
};

#endif // MODBUSCODEC_H
