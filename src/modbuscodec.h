#ifndef MODBUSCODEC_H
#define MODBUSCODEC_H

#include <QByteArray>
#include "modbusdefs.h"

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
