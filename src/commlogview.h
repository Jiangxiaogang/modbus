#ifndef COMMLOGVIEW_H
#define COMMLOGVIEW_H

#include <QWidget>
#include <QList>

class QPlainTextEdit;
class QMenu;
class QTimer;

// 单条待显示的日志行
struct LogLine
{
    QString tag;   // 类别标签
    QString color; // 类别颜色
    QString ts;    // 毫秒时间戳
    QString text;  // 报文 / 描述
};

// 底部通信日志窗口：毫秒时间戳 + 收/发/读/写组合标签(4色) + 十六进制报文(字节间空格分隔)
// 右键菜单提供"清空"；选中文字时暂停自动滚动，便于查看历史报文
// 自动滚动只动垂直滚动条，不影响水平条；批量刷新合并重绘，减少闪烁
class CommLogView : public QWidget
{
    Q_OBJECT
public:
    explicit CommLogView(QWidget *parent = 0);

public slots:
    void appendTx(bool isWrite, const QString &ts, const QString &hex);
    void appendRx(bool isWrite, const QString &ts, const QString &hex);
    void appendError(const QString &ts, const QString &msg);
    void appendInfo(const QString &ts, const QString &msg);
    void clearLog();

private slots:
    void showContextMenu(const QPoint &pos);
    void flushPending();
    void scrollToEnd();

private:
    void appendLine(const QString &tag, const QString &color,
                    const QString &ts, const QString &text);

    QPlainTextEdit *m_edit;
    QMenu          *m_menu;
    QTimer         *m_flushTimer; // 批量刷新定时器
    QList<LogLine>  m_pending;    // 待显示行缓冲
};

#endif // COMMLOGVIEW_H
