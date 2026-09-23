#include "commlogview.h"

#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QColor>
#include <QMenu>
#include <QAction>
#include <QTimer>

// 收/发 × 读/写 组合颜色
static const char *kTxReadColor  = "#0000C0"; // 发·读 蓝
static const char *kTxWriteColor = "#C000C0"; // 发·写 紫红
static const char *kRxReadColor  = "#008000"; // 收·读 绿
static const char *kRxWriteColor = "#C06000"; // 收·写 橙
static const char *kErrColor     = "#C00000"; // 错误 红
static const char *kInfoColor    = "#808080"; // 信息 灰

// 批量刷新参数：定时刷新间隔，以及积压行数达到阈值后立即刷新
static const int kFlushInterval  = 50;  // ms
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
    m_edit->setMaximumBlockCount(2000); // 自动裁剪旧日志，防止无限增长
    m_edit->setLineWrapMode(QPlainTextEdit::NoWrap);

    // 右键菜单：清空
    m_edit->setContextMenuPolicy(Qt::CustomContextMenu);
    m_menu = new QMenu(m_edit);
    connect(m_menu->addAction("清空"), &QAction::triggered, this, &CommLogView::clearLog);
    connect(m_edit, &QPlainTextEdit::customContextMenuRequested, this,
            [this](const QPoint &pos){ m_menu->exec(m_edit->mapToGlobal(pos)); });

    // 批量刷新：合并一段时间内的多次追加为一次重绘，减少高频轮询下的闪烁
    m_flushTimer = new QTimer(this);
    m_flushTimer->setSingleShot(true);
    m_flushTimer->setInterval(kFlushInterval);
    connect(m_flushTimer, &QTimer::timeout, this, &CommLogView::flushPending);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_edit);
}

void CommLogView::appendEvent(const CommEvent &ev)
{
    m_pending.append(ev);

    // 积压达到阈值立即刷新；否则由定时器合并刷新（限制重绘频率）
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

    // 关闭重绘，批量写入后一次性刷新；纯文本光标插入比逐行 HTML 解析更快
    m_edit->setUpdatesEnabled(false);

    QTextCursor cur(m_edit->document());
    cur.movePosition(QTextCursor::End);
    cur.beginEditBlock();
    // 空文档时当前块为空，直接写入，避免顶部出现空行
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
    // 用户选中文字(正在查看/复制)时不抢滚动；只滚垂直条，不动水平条
    if (!m_edit->textCursor().hasSelection())
        m_edit->verticalScrollBar()->setValue(m_edit->verticalScrollBar()->maximum());
}
