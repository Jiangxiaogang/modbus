#include <QApplication>
#include <QList>
#include "mainwindow.h"
#include "modbusdefs.h"
#include "commevent.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // Qt5 源字符串一律按 UTF-8 解释，无需再设置 QTextCodec
    QApplication::setApplicationName("ModbusTool");

    // writeRequested(int,int,DataType,ByteOrder,qint64) 为跨线程排队连接，须注册枚举类型
    qRegisterMetaType<DataType>("DataType");
    qRegisterMetaType<ByteOrder>("ByteOrder");
    // configChanged(ModbusConfig) 为跨线程排队连接，须注册结构体类型
    qRegisterMetaType<ModbusConfig>("ModbusConfig");
    // planChanged(int,QList<RegPlanItem>*) 为跨线程排队连接，须注册指针类型
    qRegisterMetaType<QList<RegPlanItem> *>("QList<RegPlanItem>*");
    // eventAppended(CommEvent) 由工作线程排队发往 UI，须注册事件类型
    qRegisterMetaType<CommEvent>("CommEvent");

    MainWindow w;
    w.show();
    return app.exec();
}
