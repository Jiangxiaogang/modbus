#ifndef VERSION_H
#define VERSION_H

// 软件版本号：主版本.次版本.修订号.构建号
// 修改版本号只需调整这四个数字，版本字符串自动生成
#define APP_VERSION_MAJOR  1
#define APP_VERSION_MINOR  0
#define APP_VERSION_PATCH  0
#define APP_VERSION_BUILD  0

#define APP_VER_STR_(x) #x
#define APP_VER_STR(x)  APP_VER_STR_(x)

// 完整版本字符串，由上面的数字宏拼接而成
#define APP_VERSION_STR   APP_VER_STR(APP_VERSION_MAJOR) "." \
                          APP_VER_STR(APP_VERSION_MINOR) "." \
                          APP_VER_STR(APP_VERSION_PATCH) "." \
                          APP_VER_STR(APP_VERSION_BUILD)

// 产品信息
#define APP_PRODUCT_NAME  "ModbusTool"
#define APP_DESCRIPTION   "Modbus 调试工具"
#define APP_COPYRIGHT     "Copyright (C) 2026"

#endif // VERSION_H
