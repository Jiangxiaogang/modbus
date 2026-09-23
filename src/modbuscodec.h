#ifndef MODBUSCODEC_H
#define MODBUSCODEC_H

#include <QByteArray>
#include <QString>

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

struct TransportConfig
{
    ChannelType  channel;

    QString portName;
    int     baudRate;
    int     parity;

    NetworkType netType;
    QString netAddr;
    int     netPort;
};

struct ModbusParams
{
    ProtocolType protocol;

    int     slave;
    int     responseTimeout;
    int     pollInterval;
    int     readMode;
    int     coilWriteFunc;
    int     regWriteFunc;
};

struct ModbusConfig
{
    TransportConfig transport;
    ModbusParams    params;
};

quint16 modbusCrc(const char *data, int len);

quint8  modbusLrc(const char *data, int len);

class ModbusCodec
{
public:

    static QByteArray encode(ProtocolType proto, quint8 slave,
                             quint8 func, const QByteArray &pdu);

    static QByteArray tryExtract(ProtocolType proto,
                                 const QByteArray &buffer, int *frameLen);

    static bool decode(ProtocolType proto, const QByteArray &frame,
                       quint8 &slave, quint8 &func, QByteArray &pdu);

    static bool isException(quint8 respFunc, quint8 reqFunc)
    {
        return (respFunc & 0x7F) == (reqFunc & 0x7F) && (respFunc & 0x80);
    }

    static QByteArray toHex(const QByteArray &bytes);

    static QString exceptionText(quint8 code);
};

#endif
