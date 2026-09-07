#ifndef COMMLOGVIEW_H
#define COMMLOGVIEW_H

#include <QWidget>

class QPlainTextEdit;
class QMenu;

// 底部通信日志窗口：毫秒时间戳 + 收/发/读/写组合标签(4色) + 十六进制报文
// 右键菜单提供"清空"
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

private:
    void appendLine(const QString &tag, const QString &color,
                    const QString &ts, const QString &text);

    QPlainTextEdit *m_edit;
    QMenu          *m_menu;
};

#endif // COMMLOGVIEW_H
