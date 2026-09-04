#include "modbusdefs.h"

quint16 modbusCrc(const char* data, int len)
{
    quint16 crc = 0xFFFF;
    for (int i = 0; i < len; ++i) {
        crc ^= (quint8)data[i];
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

quint8 modbusLrc(const char* data, int len)
{
    int sum = 0;
    for (int i = 0; i < len; ++i)
        sum += (quint8)data[i];
    return (quint8)(((sum ^ 0xFF) + 1) & 0xFF);
}

QString formatAddr(int protoAddr)
{
    return QString("0x%1").arg((quint16)protoAddr, 4, 16, QLatin1Char('0'));
}
