#include "modbuscodec.h"
#include "modbusdefs.h"

static QByteArray toHexBytes(const char* data, int len)
{
    static const char* hex = "0123456789ABCDEF";
    QByteArray out;
    out.reserve(len * 2);
    for (int i = 0; i < len; ++i) {
        quint8 c = (quint8)data[i];
        out.append(hex[(c >> 4) & 0xF]);
        out.append(hex[c & 0xF]);
    }
    return out;
}

static bool fromHexBytes(const QByteArray& hex, QByteArray& out)
{
    if (hex.size() % 2 != 0)
        return false;
    out.clear();
    out.reserve(hex.size() / 2);
    for (int i = 0; i < hex.size(); i += 2) {
        bool ok1 = false, ok2 = false;
        int hi = QChar(hex[i]).digitValue();
        int lo = QChar(hex[i + 1]).digitValue();
        if (hi < 0 || hi > 15) ok1 = false; else ok1 = true;
        if (lo < 0 || lo > 15) ok2 = false; else ok2 = true;
        if (!ok1 || !ok2) return false;
        out.append((char)((hi << 4) | lo));
    }
    return true;
}

QByteArray ModbusCodec::encode(ProtocolType proto, quint8 slave,
                               quint8 func, const QByteArray& pdu)
{
    QByteArray frame;
    frame.append((char)slave);
    frame.append((char)func);
    frame.append(pdu);

    if (proto == ProtocolRTU) {
        quint16 crc = modbusCrc(frame.constData(), frame.size());
        frame.append((char)(crc & 0xFF));        // CRC Lo
        frame.append((char)((crc >> 8) & 0xFF)); // CRC Hi
    } else if (proto == ProtocolASCII) {
        QByteArray body = frame;                 // 含从站/功能码/PDU
        quint8 lrc = modbusLrc(body.constData(), body.size());
        QByteArray out;
        out.append(':');
        out.append(toHexBytes(body.constData(), body.size()));
        out.append(toHexBytes((const char*)&lrc, 1));
        out.append('\r');
        out.append('\n');
        return out;
    } else { // ProtocolTCP
        QByteArray mbap;
        mbap.append((char)0x00); mbap.append((char)0x01); // 事务ID(占位)
        mbap.append((char)0x00); mbap.append((char)0x00); // 协议ID = 0
        int len = frame.size();                  // 单元ID + PDU
        mbap.append((char)((len >> 8) & 0xFF));
        mbap.append((char)(len & 0xFF));
        return mbap + frame;                     // frame 这里已含单元ID(slave)
    }
    return frame;
}

QByteArray ModbusCodec::tryExtract(ProtocolType proto,
                                   const QByteArray& buffer, int* frameLen)
{
    *frameLen = 0;
    if (buffer.isEmpty())
        return QByteArray();

    if (proto == ProtocolRTU) {
        // 传输层已按 3.5 字符间隔成帧，整段即一帧；最小长度 4 (从站+功能+数据+crc 至少)
        if (buffer.size() < 4)
            return QByteArray();
        *frameLen = buffer.size();
        return buffer;
    } else if (proto == ProtocolASCII) {
        // 找到 ':' ... CRLF
        int start = buffer.indexOf(':');
        if (start < 0)
            return QByteArray();
        int end = buffer.indexOf("\r\n", start);
        if (end < 0)
            return QByteArray(); // 还不完整
        *frameLen = end + 2 - start;
        return buffer.mid(start, end + 2 - start);
    } else { // ProtocolTCP
        if (buffer.size() < 6)
            return QByteArray();
        int len = ((quint8)buffer[4] << 8) | (quint8)buffer[5];
        int total = 6 + len;
        if (buffer.size() < total)
            return QByteArray();
        *frameLen = total;
        return buffer.left(total);
    }
}

bool ModbusCodec::decode(ProtocolType proto, const QByteArray& frame,
                         quint8& slave, quint8& func, QByteArray& pdu)
{
    slave = func = 0;
    pdu.clear();
    if (frame.isEmpty())
        return false;

    if (proto == ProtocolRTU) {
        if (frame.size() < 4)
            return false;
        // 校验 CRC
        QByteArray body = frame.left(frame.size() - 2);
        quint16 crc = modbusCrc(body.constData(), body.size());
        quint16 got = (quint8)frame[frame.size() - 1];
        got = (got << 8) | (quint8)frame[frame.size() - 2];
        if (crc != got)
            return false;
        slave = (quint8)frame[0];
        func  = (quint8)frame[1];
        pdu   = frame.mid(2, frame.size() - 4);
        return true;
    } else if (proto == ProtocolASCII) {
        if (frame[0] != ':' || !frame.endsWith("\r\n"))
            return false;
        QByteArray hex = frame.mid(1, frame.size() - 3); // 去掉 ':' 和 CRLF
        QByteArray bytes;
        if (!fromHexBytes(hex, bytes) || bytes.size() < 3)
            return false;
        quint8 lrc = modbusLrc(bytes.constData(), bytes.size() - 1);
        if (lrc != (quint8)bytes[bytes.size() - 1])
            return false;
        slave = (quint8)bytes[0];
        func  = (quint8)bytes[1];
        pdu   = bytes.mid(2, bytes.size() - 3);
        return true;
    } else { // ProtocolTCP
        if (frame.size() < 7)
            return false;
        int len = ((quint8)frame[4] << 8) | (quint8)frame[5];
        if (frame.size() < 6 + len)
            return false;
        slave = (quint8)frame[6];
        func  = (quint8)frame[7];
        pdu   = frame.mid(8);   // 剩余即 PDU (func 之后)
        return true;
    }
}

QString ModbusCodec::exceptionText(quint8 code)
{
    switch (code) {
    case 0x01: return "非法功能码";
    case 0x02: return "非法数据地址";
    case 0x03: return "非法数据值";
    case 0x04: return "从站设备故障";
    case 0x05: return "确认";
    case 0x06: return "从站忙";
    case 0x08: return "存储奇偶错误";
    case 0x0A: return "网关路径不可用";
    case 0x0B: return "网关目标设备无响应";
    default:   return QString("异常码 0x%1").arg(code, 2, 16, QLatin1Char('0'));
    }
}
