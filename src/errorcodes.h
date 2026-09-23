#ifndef ERRORCODES_H
#define ERRORCODES_H

#include <QString>

enum class ErrorCode
{
    Ok = 0,
    NotConnected,
    UnsupportedChannel,
    ResponseTimeout,
    FrameParseFailed,
    FrameLengthError,
    EmptyResponse,
    SlaveException,
    FunctionMismatch,
    ConnectFailed,
    SerialOpenFailed,
    SerialWriteFailed,
    SerialRecvTimeout,
    TcpConnectFailed,
    TcpWriteFailed,
    TcpRecvTimeout,
    UdpBindFailed,
    UdpWriteFailed,
    UdpRecvTimeout,
    ConnectionLost
};

QString errorText(ErrorCode code);
QString errorText(ErrorCode code, const QString &detail);
QString errorText(ErrorCode code, const QString &detail1, const QString &detail2);

#endif
