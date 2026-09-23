#include "commlogview.h"
#include "modbuscodec.h"

#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QColor>
#include <QMenu>
#include <QAction>
#include <QTimer>

static const char *kTxReadColor  = "#0000C0";
static const char *kTxWriteColor = "#C000C0";
static const char *kRxReadColor  = "#008000";
static const char *kRxWriteColor = "#C06000";
static const char *kErrColor     = "#C00000";
static const char *kInfoColor    = "#808080";

static const int kFlushInterval  = 50;
static const int kFlushLineCount = 50;

static QString eventTag(const CommEvent &ev)
{
    switch (ev.dir)
    {
    case CommTx:    return "发送";
    case CommRx:    return "接收";
    case CommError: return "错误";
    default:        return "信息";
    }
}

static const char *eventColor(const CommEvent &ev)
{
    switch (ev.dir)
    {
    case CommTx:    return ev.isWrite ? kTxWriteColor : kTxReadColor;
    case CommRx:    return ev.isWrite ? kRxWriteColor : kRxReadColor;
    case CommError: return kErrColor;
    default:        return kInfoColor;
    }
}

CommLogView::CommLogView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(80);

    m_edit = new QPlainTextEdit(this);
    m_edit->setReadOnly(true);
    m_edit->setUndoRedoEnabled(false);
    m_edit->setMaximumBlockCount(2000);
    m_edit->setLineWrapMode(QPlainTextEdit::NoWrap);

    m_edit->setContextMenuPolicy(Qt::CustomContextMenu);
    m_menu = new QMenu(m_edit);
    connect(m_menu->addAction("清空"), &QAction::triggered, this, &CommLogView::clearLog);
    connect(m_edit, &QPlainTextEdit::customContextMenuRequested, this,
            [this](const QPoint &pos){ m_menu->exec(m_edit->mapToGlobal(pos)); });

    m_flushTimer = new QTimer(this);
    m_flushTimer->setSingleShot(true);
    m_flushTimer->setInterval(kFlushInterval);
    connect(m_flushTimer, &QTimer::timeout, this, &CommLogView::flushPending);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_edit);
}

bool CommLogView::funcIsWrite(quint8 func)
{
    return func == 5 || func == 6 || func == 15 || func == 16;
}

QString CommLogView::frameText(const QByteArray &frame)
{
    return QString::fromLatin1(ModbusCodec::toHex(frame));
}

void CommLogView::appendFrame(bool tx, const QByteArray &frame, quint8 func)
{
    CommEvent ev;
    ev.dir     = tx ? CommTx : CommRx;
    ev.time    = QTime::currentTime();
    ev.frame   = frame;
    ev.func    = func;
    ev.isWrite = funcIsWrite(func);
    ev.text    = frameText(frame);
    appendEvent(ev);
}

void CommLogView::appendInfo(const QString &text)
{
    CommEvent ev;
    ev.dir  = CommInfo;
    ev.time = QTime::currentTime();
    ev.text = text;
    appendEvent(ev);
}

void CommLogView::appendError(const QString &text)
{
    CommEvent ev;
    ev.dir  = CommError;
    ev.time = QTime::currentTime();
    ev.text = text;
    appendEvent(ev);
}

void CommLogView::appendEvent(const CommEvent &ev)
{
    m_pending.append(ev);

    if (m_pending.size() >= kFlushLineCount)
    {
        m_flushTimer->stop();
        flushPending();
    }
    else if (!m_flushTimer->isActive())
    {
        m_flushTimer->start();
    }
}

void CommLogView::clearLog()
{
    m_flushTimer->stop();
    m_pending.clear();
    m_edit->clear();
}

void CommLogView::flushPending()
{
    if (m_pending.isEmpty())
        return;

    m_edit->setUpdatesEnabled(false);

    QTextCursor cur(m_edit->document());
    cur.movePosition(QTextCursor::End);
    cur.beginEditBlock();

    bool needBlock = !m_edit->document()->isEmpty();
    for (const CommEvent &ev : m_pending)
    {
        if (needBlock)
            cur.insertBlock();
        needBlock = true;
        QTextCharFormat fmt;
        fmt.setForeground(QColor(eventColor(ev)));
        cur.insertText(QString("[%1] [%2] %3")
                       .arg(ev.time.toString("hh:mm:ss.zzz"))
                       .arg(eventTag(ev))
                       .arg(ev.text), fmt);
    }
    cur.endEditBlock();
    m_pending.clear();

    m_edit->setUpdatesEnabled(true);

    if (!m_edit->textCursor().hasSelection())
        m_edit->verticalScrollBar()->setValue(m_edit->verticalScrollBar()->maximum());
}
