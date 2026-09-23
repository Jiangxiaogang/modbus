#ifndef COMMMONITOR_H
#define COMMMONITOR_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QMutex>

struct CommRecord
{
    enum Direction { Tx = 0, Rx, Error, Info };

    Direction  dir = Info;
    QDateTime  time;
    QByteArray frame;
    quint8     func = 0;
    QString    text;

    bool isWrite() const
    {
        return func == 5 || func == 6 || func == 15 || func == 16;
    }
};

class CommMonitor : public QObject
{
    Q_OBJECT
public:
    explicit CommMonitor(QObject *parent = nullptr);

    void recordTx(const QByteArray &frame, quint8 func);
    void recordRx(const QByteArray &frame, quint8 func);
    void recordError(const QString &text, quint8 modbusErr = 0);
    void recordInfo(const QString &text);

    void resetCounters();
    void clearHistory();

    quint32 txCount() const;
    quint32 rxCount() const;
    quint32 errorCount() const;

    QList<CommRecord> history() const;
    int  historyLimit() const;
    void setHistoryLimit(int limit);

signals:
    void frameSent(const QByteArray &frame, quint8 func);
    void frameReceived(const QByteArray &frame, quint8 func);
    void errorLogged(const QString &text, quint8 modbusErr);
    void infoLogged(const QString &text);
    void countersChanged(quint32 tx, quint32 rx, quint32 errors);

private:
    void appendLocked(const CommRecord &rec);

    mutable QMutex    m_mutex;
    QList<CommRecord> m_history;
    int     m_limit = 2000;
    quint32 m_tx  = 0;
    quint32 m_rx  = 0;
    quint32 m_err = 0;
};

#endif
