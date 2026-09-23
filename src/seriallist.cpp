#include "seriallist.h"
#include <QSerialPortInfo>

QStringList SerialList::getSerialPorts()
{
    QStringList ports;
    const QList<QSerialPortInfo> list = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : list)
        ports.append(info.portName());
    if (ports.isEmpty())
        ports.append("COM1");
    return ports;
}
