#include "commlogview.h"

#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QColor>
#include <QMenu>
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
    connect(m_edit, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showContextMenu(QPoint)));
    m_menu = new QMenu(m_edit);
    m_menu->addAction("清空", this, SLOT(clearLog()));

    // 批量刷新：合并一段时间内的多次追加为一次重绘，减少高频轮询下的闪烁
    m_flushTimer = new QTimer(this);
    m_flushTimer->setSingleShot(true);
    m_flushTimer->setInterval(kFlushInterval);
    connect(m_flushTimer, SIGNAL(timeout()), this, SLOT(flushPending()));

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_edit);
}

void CommLogView::appendTx(bool isWrite, const QString &ts, const QString &hex)
{
    appendLine(isWrite ? "发送" : "发送",
               isWrite ? kTxWriteColor : kTxReadColor, ts, hex);
}

void CommLogView::appendRx(bool isWrite, const QString &ts, const QString &hex)
{
    appendLine(isWrite ? "接收" : "接收",
               isWrite ? kRxWriteColor : kRxReadColor, ts, hex);
}

void CommLogView::appendError(const QString &ts, const QString &msg)
{
    appendLine("错误", kErrColor, ts, msg);
}

void CommLogView::appendInfo(const QString &ts, const QString &msg)
{
    appendLine("信息", kInfoColor, ts, msg);
}

void CommLogView::clearLog()
{
    m_flushTimer->stop();
    m_pending.clear();
    m_edit->clear();
}

void CommLogView::showContextMenu(const QPoint &pos)
{
    m_menu->exec(m_edit->mapToGlobal(pos));
}

void CommLogView::appendLine(const QString &tag, const QString &color,
                             const QString &ts, const QString &text)
{
    LogLine line;
    line.tag   = tag;
    line.color = color;
    line.ts    = ts;
    line.text  = text;
    m_pending.append(line);

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
    foreach (const LogLine &line, m_pending)
    {
        if (needBlock)
            cur.insertBlock();
        needBlock = true;
        QTextCharFormat fmt;
        fmt.setForeground(QColor(line.color));
        cur.insertText(QString("[%1] [%2] %3")
                       .arg(line.ts).arg(line.tag).arg(line.text), fmt);
    }
    cur.endEditBlock();
    m_pending.clear();

    m_edit->setUpdatesEnabled(true);
    scrollToEnd();
}

void CommLogView::scrollToEnd()
{
    // 用户选中文字(正在查看/复制)时不抢滚动；只滚垂直条，不动水平条
    if (m_edit->textCursor().hasSelection())
        return;
    QScrollBar *vsb = m_edit->verticalScrollBar();
    vsb->setValue(vsb->maximum());
}
