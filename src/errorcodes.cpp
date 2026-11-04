#include "errorcodes.h"

// 注意：MSVC + /utf-8 下，QStringLiteral 对中文会因 u"" 拼接缺陷产生乱码，
// 这里统一使用 QString("...")（Qt5 对窄字符串默认按 UTF-8 解析）。

QString errorText(ErrorCode code)
{
    switch (code)
    {
    case ErrorCode::Ok:                 return QString();
    case ErrorCode::NotConnected:       return QString("设备未连接");
    case ErrorCode::UnsupportedChannel: return QString("不支持的通道配置");
    case ErrorCode::ResponseTimeout:    return QString("设备响应超时");
    case ErrorCode::FrameParseFailed:   return QString("响应解析失败(校验/格式错误)");
    case ErrorCode::FrameLengthError:   return QString("响应帧长度错误");
    case ErrorCode::EmptyResponse:      return QString("响应数据为空");
    case ErrorCode::SlaveException:     return QString("从站异常: %1");
    case ErrorCode::FunctionMismatch:   return QString("功能码不匹配 请求0x%1 响应0x%2");
    case ErrorCode::SlaveMismatch:      return QString("从站地址不匹配 期望%1 实际%2");
    case ErrorCode::ConnectFailed:      return QString("连接失败: %1");
    case ErrorCode::SerialOpenFailed:   return QString("打开串口失败");
    case ErrorCode::SerialWriteFailed:  return QString("串口写入失败");
    case ErrorCode::SerialRecvTimeout:  return QString("串口接收超时");
    case ErrorCode::TcpConnectFailed:   return QString("TCP 连接失败");
    case ErrorCode::TcpWriteFailed:     return QString("TCP 写入失败");
    case ErrorCode::TcpRecvTimeout:     return QString("TCP 接收超时");
    case ErrorCode::UdpBindFailed:      return QString("UDP 绑定失败");
    case ErrorCode::UdpWriteFailed:     return QString("UDP 写入失败");
    case ErrorCode::UdpRecvTimeout:     return QString("UDP 接收超时");
    case ErrorCode::ConnectionLost:     return QString("连接已断开");
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
