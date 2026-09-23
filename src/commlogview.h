#ifndef COMMLOGVIEW_H
#define COMMLOGVIEW_H

#include <QWidget>
#include <QList>
#include <QByteArray>
#include <QString>
#include <QTime>

class QPlainTextEdit;
class QMenu;
class QTimer;

enum CommDir
{
    CommTx = 0,
    CommRx,
    CommError,
    CommInfo
};

struct CommEvent
{
    CommDir     dir = CommInfo;
    QTime       time;
    QByteArray  frame;
    quint8      func = 0;
    bool        isWrite = false;
    QString     text;
};

class CommLogView : public QWidget
{
    Q_OBJECT
public:
    explicit CommLogView(QWidget *parent = nullptr);

public slots:
    void appendFrame(bool tx, const QByteArray &frame, quint8 func);
    void appendInfo(const QString &text);
    void appendError(const QString &text);
    void clearLog();

private slots:
    void flushPending();

private:
    void appendEvent(const CommEvent &ev);
    static bool funcIsWrite(quint8 func);
    static QString frameText(const QByteArray &frame);

    QPlainTextEdit  *m_edit;
    QMenu           *m_menu;
    QTimer          *m_flushTimer;
    QList<CommEvent> m_pending;
};

#endif
