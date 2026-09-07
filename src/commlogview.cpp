#include "commlogview.h"

#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QMenu>

// 收/发 × 读/写 组合颜色
static const char *kTxReadColor  = "#0000C0"; // 发·读 蓝
static const char *kTxWriteColor = "#C000C0"; // 发·写 紫红
static const char *kRxReadColor  = "#008000"; // 收·读 绿
static const char *kRxWriteColor = "#C06000"; // 收·写 橙
static const char *kErrColor     = "#C00000"; // 错误 红
static const char *kInfoColor    = "#808080"; // 信息 灰

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

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_edit);
}

void CommLogView::appendTx(bool isWrite, const QString &ts, const QString &hex)
{
    appendLine(isWrite ? "发·写" : "发·读",
               isWrite ? kTxWriteColor : kTxReadColor, ts, hex);
}

void CommLogView::appendRx(bool isWrite, const QString &ts, const QString &hex)
{
    appendLine(isWrite ? "收·写" : "收·读",
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
    m_edit->clear();
}

void CommLogView::showContextMenu(const QPoint &pos)
{
    m_menu->exec(m_edit->mapToGlobal(pos));
}

void CommLogView::appendLine(const QString &tag, const QString &color,
                             const QString &ts, const QString &text)
{
    // 整行使用类别颜色：[hh:mm:ss.zzz] [发·读] 01 03 00 00 ...
    QString html = QString("<span style=\"color:%1\">[%2] [%3] %4</span>")
                   .arg(color).arg(ts).arg(tag).arg(text);
    m_edit->appendHtml(html);
    // 追加后自动滚动到底部
    QTextCursor c = m_edit->textCursor();
    c.movePosition(QTextCursor::End);
    m_edit->setTextCursor(c);
}
