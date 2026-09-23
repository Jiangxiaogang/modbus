#include "errorcodes.h"

QString errorText(ErrorCode code)
{
    switch (code)
    {
    case ErrorCode::Ok:                 return QString();
    case ErrorCode::NotConnected:       return QStringLiteral("设备未连接");
    case ErrorCode::UnsupportedChannel: return QStringLiteral("不支持的通道配置");
    case ErrorCode::ResponseTimeout:    return QStringLiteral("设备响应超时");
    case ErrorCode::FrameParseFailed:   return QStringLiteral("响应解析失败(校验/格式错误)");
    case ErrorCode::FrameLengthError:   return QStringLiteral("响应帧长度错误");
    case ErrorCode::EmptyResponse:      return QStringLiteral("响应数据为空");
    case ErrorCode::SlaveException:     return QStringLiteral("从站异常: %1");
    case ErrorCode::FunctionMismatch:   return QStringLiteral("功能码不匹配 请求0x%1 响应0x%2");
    case ErrorCode::ConnectFailed:      return QStringLiteral("连接失败: %1");
    case ErrorCode::SerialOpenFailed:   return QStringLiteral("打开串口失败");
    case ErrorCode::SerialWriteFailed:  return QStringLiteral("串口写入失败");
    case ErrorCode::SerialRecvTimeout:  return QStringLiteral("串口接收超时");
    case ErrorCode::TcpConnectFailed:   return QStringLiteral("TCP 连接失败");
    case ErrorCode::TcpWriteFailed:     return QStringLiteral("TCP 写入失败");
    case ErrorCode::TcpRecvTimeout:     return QStringLiteral("TCP 接收超时");
    case ErrorCode::UdpBindFailed:      return QStringLiteral("UDP 绑定失败");
    case ErrorCode::UdpWriteFailed:     return QStringLiteral("UDP 写入失败");
    case ErrorCode::UdpRecvTimeout:     return QStringLiteral("UDP 接收超时");
    case ErrorCode::ConnectionLost:     return QStringLiteral("连接已断开");
    }
    return QString();
}

QString errorText(ErrorCode code, const QString &detail)
{
    return errorText(code).arg(detail);
}

QString errorText(ErrorCode code, const QString &detail1, const QString &detail2)
{
    return errorText(code).arg(detail1, detail2);
}
