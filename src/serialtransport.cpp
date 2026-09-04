#include "serialtransport.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

SerialTransport::SerialTransport(const QString &portName, int baudRate,
                                 int dataBits, int stopBits, char parity)
    : m_portName(portName), m_baudRate(baudRate), m_dataBits(dataBits)
    , m_stopBits(stopBits), m_parity(parity), m_handle(INVALID_HANDLE_VALUE)
{
}

SerialTransport::~SerialTransport()
{
    close();
}

bool SerialTransport::open()
{
    QString name = m_portName;
    if (!name.startsWith("\\\\.\\"))
        name = "\\\\.\\" + name;
    HANDLE h = CreateFileA(name.toLocal8Bit().constData(),
                           GENERIC_READ | GENERIC_WRITE,
                           0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        m_err = QString("打开串口失败 (0x%1)").arg((int)GetLastError(), 0, 16);
        return false;
    }
    m_handle = h;

    DCB dcb;
    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    dcb.BaudRate = m_baudRate;
    dcb.ByteSize = (BYTE)m_dataBits;
    switch (m_stopBits)
    {
    case 20:
        dcb.StopBits = TWOSTOPBITS;
        break;
    case 15:
        dcb.StopBits = ONE5STOPBITS;
        break;
    default:
        dcb.StopBits = ONESTOPBIT;
        break;
    }
    switch (m_parity)
    {
    case 'E':
        dcb.Parity = EVENPARITY;
        dcb.fParity = TRUE;
        break;
    case 'O':
        dcb.Parity = ODDPARITY;
        dcb.fParity = TRUE;
        break;
    case 'M':
        dcb.Parity = MARKPARITY;
        dcb.fParity = TRUE;
        break;
    case 'S':
        dcb.Parity = SPACEPARITY;
        dcb.fParity = TRUE;
        break;
    default:
        dcb.Parity = NOPARITY;
        dcb.fParity = FALSE;
        break;
    }
    dcb.fBinary        = TRUE;
    dcb.fAbortOnError  = FALSE;
    dcb.fDtrControl    = DTR_CONTROL_ENABLE;
    dcb.fRtsControl    = RTS_CONTROL_ENABLE;
    if (!SetCommState(h, &dcb))
    {
        m_err = "配置串口参数失败";
        close();
        return false;
    }

    // 3.5 字符时间间隔（用于 RTU/ASCII 帧边界检测）
    double charBits = 11.0; // 8N1
    if (m_parity != 'N') charBits = 12.0;
    double charMs = (charBits * 1000.0) / (double)m_baudRate;
    DWORD interval = (DWORD)(charMs * 3.5);
    if (interval < 1) interval = 1;

    COMMTIMEOUTS ct;
    memset(&ct, 0, sizeof(ct));
    ct.ReadIntervalTimeout         = interval;     // 字符间超时 -> 帧结束
    ct.ReadTotalTimeoutMultiplier  = 0;
    ct.ReadTotalTimeoutConstant    = 1000;         // 占位，open 时未知超时
    ct.WriteTotalTimeoutMultiplier = 0;
    ct.WriteTotalTimeoutConstant   = 1000;
    SetCommTimeouts(h, &ct);

    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
    return true;
}

void SerialTransport::close()
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        PurgeComm((HANDLE)m_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
        CloseHandle((HANDLE)m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

bool SerialTransport::isOpen() const
{
    return m_handle != INVALID_HANDLE_VALUE;
}

qint64 SerialTransport::write(const char *data, qint64 len)
{
    if (!isOpen()) return -1;
    PurgeComm((HANDLE)m_handle, PURGE_RXCLEAR); // 清掉旧数据
    DWORD written = 0;
    if (!WriteFile((HANDLE)m_handle, data, (DWORD)len, &written, NULL))
    {
        m_err = "串口写入失败";
        return -1;
    }
    // RTU 发送后等待 3.5 字符时间，保证帧间隔
    double charBits = (m_parity == 'N') ? 11.0 : 12.0;
    int ms = (int)(charBits * 1000.0 / (double)m_baudRate * 3.5) + 1;
    Sleep(ms);
    return (qint64)written;
}

QByteArray SerialTransport::read(int timeoutMs)
{
    QByteArray buf;
    if (!isOpen())
        return buf;
    HANDLE h = (HANDLE)m_handle;

    COMMTIMEOUTS ct;
    memset(&ct, 0, sizeof(ct));
    double charBits = (m_parity == 'N') ? 11.0 : 12.0;
    double charMs = (charBits * 1000.0) / (double)m_baudRate;
    DWORD interval = (DWORD)(charMs * 3.5);
    if (interval < 1) interval = 1;
    ct.ReadIntervalTimeout         = interval;
    ct.ReadTotalTimeoutMultiplier  = 0;
    ct.ReadTotalTimeoutConstant    = (DWORD)timeoutMs;
    SetCommTimeouts(h, &ct);

    DWORD start = GetTickCount();
    while ((int)(GetTickCount() - start) < timeoutMs + 50)
    {
        char tmp[256];
        DWORD got = 0;
        if (!ReadFile(h, tmp, sizeof(tmp), &got, NULL))
        {
            m_err = "串口读取失败";
            break;
        }
        if (got == 0)
        {
            // 发生间隔超时，说明一帧已结束
            if (!buf.isEmpty())
                break;
            // 还没读到任何数据且超时 -> 返回空
            if ((int)(GetTickCount() - start) >= timeoutMs)
                break;
            continue;
        }
        buf.append(tmp, (int)got);
    }
    return buf;
}

QString SerialTransport::errorString() const
{
    return m_err;
}
