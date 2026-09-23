#ifndef COMMLOGVIEW_H
#define COMMLOGVIEW_H

#include "commevent.h"
#include <QWidget>
#include <QList>

class QPlainTextEdit;
class QMenu;
class QTimer;

// 底部通信日志窗口：毫秒时间戳 + 收/发/读/写组合标签(4色) + 十六进制报文(字节间空格分隔)
// 输入为统一的 CommEvent；右键菜单提供"清空"；选中文字时暂停自动滚动；
// 批量刷新合并重绘，减少闪烁；自动裁剪最近 2000 行。
class CommLogView : public QWidget
{
    Q_OBJECT
public:
    explicit CommLogView(QWidget *parent = nullptr);

public slots:
    void appendEvent(const CommEvent &ev);
    void clearLog();

private slots:
    void flushPending();

private:
    QPlainTextEdit  *m_edit;
    QMenu           *m_menu;
    QTimer          *m_flushTimer; // 批量刷新定时器
    QList<CommEvent> m_pending;    // 待显示事件缓冲
};

#endif // COMMLOGVIEW_H
