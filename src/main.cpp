#include <QApplication>
#include <QTextCodec>
#include "mainwindow.h"
#include "modbusdefs.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QApplication::setApplicationName("ModbusTool");

    // writeRequested(int,int,DataType,qint64) 为跨线程排队连接，须注册枚举类型
    qRegisterMetaType<DataType>("DataType");

    MainWindow w;
    w.show();
    return app.exec();
}
