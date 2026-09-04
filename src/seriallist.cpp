
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "seriallist.h"
#include <QStringList>

QStringList SerialList::getSerialPorts()
{
    QStringList ports;
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        char name[256];
        char val[256];
        DWORD nameLen, valLen, type;
        for (DWORD i = 0; ; ++i)
        {
            nameLen = sizeof(name);
            valLen = sizeof(val);
            LONG r = RegEnumValueA(hKey, i, name, &nameLen, 0, &type, (LPBYTE)val, &valLen);
            if (r != ERROR_SUCCESS) break;
            ports.append(QString::fromLocal8Bit(val, valLen));
        }
        RegCloseKey(hKey);
    }
    if (ports.isEmpty())
    {
        for (int i = 1; i <= 20; ++i)
            ports.append(QString("COM%1").arg(i));
    }
    return ports;
}
