#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

// “关于”对话框：显示产品名、版本号、构建环境与版权信息（F1 弹出）
class AboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = 0);
};

#endif // ABOUTDIALOG_H
