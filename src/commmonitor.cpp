#include "commmonitor.h"

CommMonitor &CommMonitor::instance()
{
    static CommMonitor inst;
    return inst;
}

CommMonitor::CommMonitor(QObject *parent)
    : QObject(parent)
{
}

void CommMonitor::appendLocked(const CommRecord &rec)
{
    m_history.append(rec);
    while (m_history.size() > m_limit)
    {
        m_history.removeFirst();
    }
}

void CommMonitor::recordTx(const QByteArray &frame, quint8 func)
{
    quint32 tx, rx, err;
    {
        QMutexLocker lock(&m_mutex);
        ++m_tx;
        CommRecord rec;
        rec.dir   = CommRecord::Tx;
        rec.time  = QDateTime::currentDateTime();
        rec.frame = frame;
        rec.func  = func;
        appendLocked(rec);
        tx = m_tx; rx = m_rx; err = m_err;
    }
    emit frameSent(frame, func);
    emit countersChanged(tx, rx, err);
}

void CommMonitor::recordRx(const QByteArray &frame, quint8 func)
{
    quint32 tx, rx, err;
    {
        QMutexLocker lock(&m_mutex);
        ++m_rx;
        CommRecord rec;
        rec.dir   = CommRecord::Rx;
        rec.time  = QDateTime::currentDateTime();
        rec.frame = frame;
        rec.func  = func;
        appendLocked(rec);
        tx = m_tx; rx = m_rx; err = m_err;
    }
    emit frameReceived(frame, func);
    emit countersChanged(tx, rx, err);
}

void CommMonitor::recordError(const QString &text, quint8 modbusErr)
{
    quint32 tx, rx, err;
    {
        QMutexLocker lock(&m_mutex);
        ++m_err;
        CommRecord rec;
        rec.dir  = CommRecord::Error;
        rec.time = QDateTime::currentDateTime();
        rec.text = text;
        appendLocked(rec);
        tx = m_tx; rx = m_rx; err = m_err;
    }
    emit errorLogged(text, modbusErr);
    emit countersChanged(tx, rx, err);
}

void CommMonitor::recordInfo(const QString &text)
{
    {
        QMutexLocker lock(&m_mutex);
        CommRecord rec;
        rec.dir  = CommRecord::Info;
        rec.time = QDateTime::currentDateTime();
        rec.text = text;
        appendLocked(rec);
    }
    emit infoLogged(text);
}

void CommMonitor::resetCounters()
{
    {
        QMutexLocker lock(&m_mutex);
        m_tx = m_rx = m_err = 0;
    }
    emit countersChanged(0, 0, 0);
}

void CommMonitor::clearHistory()
{
    QMutexLocker lock(&m_mutex);
    m_history.clear();
}

quint32 CommMonitor::txCount() const
{
    QMutexLocker lock(&m_mutex);
    return m_tx;
}

quint32 CommMonitor::rxCount() const
{
    QMutexLocker lock(&m_mutex);
    return m_rx;
}

quint32 CommMonitor::errorCount() const
{
    QMutexLocker lock(&m_mutex);
    return m_err;
}

QList<CommRecord> CommMonitor::history() const
{
    QMutexLocker lock(&m_mutex);
    return m_history;
}

int CommMonitor::historyLimit() const
{
    QMutexLocker lock(&m_mutex);
    return m_limit;
}

void CommMonitor::setHistoryLimit(int limit)
{
    if (limit < 1)
    {
        limit = 1;
    }
    QMutexLocker lock(&m_mutex);
    m_limit = limit;
    while (m_history.size() > m_limit)
    {
        m_history.removeFirst();
    }
}
