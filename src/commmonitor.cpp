#include "commmonitor.h"
#include "modbuscodec.h"

CommMonitor::CommMonitor(QObject *parent)
    : QObject(parent)
{
}

bool CommMonitor::funcIsWrite(quint8 func)
{
    return func == 5 || func == 6 || func == 15 || func == 16;
}

QString CommMonitor::frameText(const QByteArray &frame)
{
    return QString::fromLatin1(ModbusCodec::toHex(frame));
}

void CommMonitor::append(const CommEvent &ev)
{
    emit eventAppended(ev);
}

void CommMonitor::emitStats()
{
    emit statsUpdated(m_tx, m_rx, m_errCount);
}

void CommMonitor::onFrameSent(const QByteArray &frame, quint8 func)
{
    ++m_tx;
    CommEvent ev;
    ev.dir     = CommTx;
    ev.time    = QTime::currentTime();
    ev.frame   = frame;
    ev.func    = func;
    ev.isWrite = funcIsWrite(func);
    ev.text    = frameText(frame);
    append(ev);
    emitStats();
}

void CommMonitor::onFrameReceived(const QByteArray &frame, quint8 func)
{
    ++m_rx;
    CommEvent ev;
    ev.dir     = CommRx;
    ev.time    = QTime::currentTime();
    ev.frame   = frame;
    ev.func    = func;
    ev.isWrite = funcIsWrite(func);
    ev.text    = frameText(frame);
    append(ev);
    emitStats();
}

void CommMonitor::onOperationError(const QString &err, quint8 modbusErr)
{
    Q_UNUSED(modbusErr);
    ++m_errCount;
    reportError(err);
    emitStats();
}

void CommMonitor::reportInfo(const QString &text)
{
    CommEvent ev;
    ev.dir  = CommInfo;
    ev.time = QTime::currentTime();
    ev.text = text;
    append(ev);
}

void CommMonitor::reportError(const QString &text)
{
    CommEvent ev;
    ev.dir  = CommError;
    ev.time = QTime::currentTime();
    ev.text = text;
    append(ev);
}

void CommMonitor::resetStats()
{
    m_tx = m_rx = m_errCount = 0;
    emitStats();
}
