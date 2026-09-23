#ifndef COMMEVENT_H
#define COMMEVENT_H

#include <QByteArray>
#include <QString>
#include <QTime>
#include <QMetaType>

// 通信事件方向：发送帧 / 接收帧 / 错误 / 提示
enum CommDir
{
    CommTx = 0,
    CommRx,
    CommError,
    CommInfo
};

// 一条通信事件（报文或信息），是监控器加工、展示与计数的统一数据对象
struct CommEvent
{
    CommDir     dir = CommInfo;
    QTime       time;              // I/O 边界时间戳
    QByteArray  frame;             // Tx/Rx 原始帧
    quint8      func = 0;          // 功能码（Tx/Rx）
    bool        isWrite = false;   // 由功能码推导：05/06/15/16
    QString     text;              // Error/Info 文本
};

Q_DECLARE_METATYPE(CommEvent)

#endif // COMMEVENT_H
