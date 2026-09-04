#include <QApplication>
#include <QTextCodec>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // 源码与字符串字面量均为 UTF-8 编码，统一告知 Qt 以此解码，避免中文乱码
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QApplication::setApplicationName("ModbusTool");
    MainWindow w;
    w.show();
    return app.exec();
}
