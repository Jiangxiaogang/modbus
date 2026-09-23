#include "modbusdevice.h"
#include "modbusclient.h"

ModbusDevice::ModbusDevice(QObject *parent)
    : QObject(parent), m_client(new ModbusClient(this))
{
    connect(m_client, &ModbusClient::frameSent, this, &ModbusDevice::frameSent);
    connect(m_client, &ModbusClient::frameReceived, this, &ModbusDevice::frameReceived);
}

ModbusDevice::~ModbusDevice() = default;

bool ModbusDevice::connectDevice(const ModbusConfig &cfg)
{
    return m_client->open(cfg);
}

void ModbusDevice::disconnectDevice()
{
    m_client->close();
}

bool ModbusDevice::isConnected() const
{
    return m_client->isOpen();
}

QString ModbusDevice::errorString() const
{
    return m_client->errorString();
}

void ModbusDevice::setConfig(const ModbusConfig &cfg)
{
    m_client->setConfig(cfg);
}

bool ModbusDevice::readRequest(int readFunc, int start, int count,
                               QByteArray &rx, QString &err, quint8 *modbusErr)
{
    QByteArray tx;
    tx.append((char)((start >> 8) & 0xFF));
    tx.append((char)(start & 0xFF));
    tx.append((char)((count >> 8) & 0xFF));
    tx.append((char)(count & 0xFF));
    bool ok = m_client->transact((quint8)readFunc, tx, rx, err, nullptr, nullptr, modbusErr);
    if (!ok)
        emit operationFailed(err, modbusErr ? *modbusErr : 0);
    return ok;
}

bool ModbusDevice::readBits(int readFunc, int start, int count,
                            QVector<bool> &values, QString &err, quint8 *modbusErr)
{
    values.clear();
    QByteArray rx;
    if (!readRequest(readFunc, start, count, rx, err, modbusErr))
        return false;
    if (rx.isEmpty())
    {
        err = "响应数据为空";
        return false;
    }
    const char *data = rx.constData() + 1;
    values.reserve(count);
    for (int i = 0; i < count; ++i)
        values.append(((data[i / 8] >> (i % 8)) & 0x01) != 0);
    return true;
}

bool ModbusDevice::readRegisters(int readFunc, int start, int count,
                                 QVector<quint16> &values, QString &err, quint8 *modbusErr)
{
    values.clear();
    QByteArray rx;
    if (!readRequest(readFunc, start, count, rx, err, modbusErr))
        return false;
    if (rx.isEmpty())
    {
        err = "响应数据为空";
        return false;
    }
    int n = (quint8)rx[0] / 2;
    const char *data = rx.constData() + 1;
    values.reserve(n);
    for (int i = 0; i < n; ++i)
        values.append((quint16)(((quint8)data[2 * i] << 8) | (quint8)data[2 * i + 1]));
    return true;
}

bool ModbusDevice::writeCoil(int address, bool value, int coilFunc, QString &err)
{
    QByteArray tx;
    tx.append((char)((address >> 8) & 0xFF));
    tx.append((char)(address & 0xFF));
    int func = coilFunc;
    if (func == 15)
    {
        tx.append((char)0x00);
        tx.append((char)0x01);
        tx.append((char)0x01);
        tx.append((char)(value ? 0x01 : 0x00));
    }
    else     // 05
    {
        func = 5;
        tx.append((char)(value ? 0xFF : 0x00));
        tx.append((char)0x00);
    }
    QByteArray rx;
    quint8 mbErr = 0;
    bool ok = m_client->transact((quint8)func, tx, rx, err, nullptr, nullptr, &mbErr);
    if (!ok)
        emit operationFailed(err, mbErr);
    return ok;
}

bool ModbusDevice::writeRegisters(int start, const QVector<quint16> &values, QString &err)
{
    QByteArray tx;
    tx.append((char)((start >> 8) & 0xFF));
    tx.append((char)(start & 0xFF));
    int qty = values.size();
    tx.append((char)((qty >> 8) & 0xFF));
    tx.append((char)(qty & 0xFF));
    tx.append((char)(qty * 2));
    for (quint16 v : values)
    {
        tx.append((char)((v >> 8) & 0xFF));
        tx.append((char)(v & 0xFF));
    }
    QByteArray rx;
    quint8 mbErr = 0;
    bool ok = m_client->transact(16, tx, rx, err, nullptr, nullptr, &mbErr);
    if (!ok)
        emit operationFailed(err, mbErr);
    return ok;
}

bool ModbusDevice::writeRegister(int address, quint16 value, int regFunc, QString &err)
{
    QByteArray tx;
    tx.append((char)((address >> 8) & 0xFF));
    tx.append((char)(address & 0xFF));
    int func = regFunc;
    if (func == 16)
    {
        tx.append((char)0x00);
        tx.append((char)0x01);
        tx.append((char)0x02);
    }
    else     // 06
    {
        func = 6;
    }
    tx.append((char)((value >> 8) & 0xFF));
    tx.append((char)(value & 0xFF));
    QByteArray rx;
    quint8 mbErr = 0;
    bool ok = m_client->transact((quint8)func, tx, rx, err, nullptr, nullptr, &mbErr);
    if (!ok)
        emit operationFailed(err, mbErr);
    return ok;
}
