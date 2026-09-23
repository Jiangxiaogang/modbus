#ifndef VERSION_H
#define VERSION_H

#define APP_VERSION_MAJOR  1
#define APP_VERSION_MINOR  1
#define APP_VERSION_PATCH  0
#define APP_VERSION_BUILD  0

#define APP_VER_STR_(x) #x
#define APP_VER_STR(x)  APP_VER_STR_(x)

#define APP_VERSION_STR   APP_VER_STR(APP_VERSION_MAJOR) "." \
                          APP_VER_STR(APP_VERSION_MINOR) "." \
                          APP_VER_STR(APP_VERSION_PATCH) "." \
                          APP_VER_STR(APP_VERSION_BUILD)

#define APP_PRODUCT_NAME  "ModbusTool"
#define APP_DESCRIPTION   "Modbus 调试工具"
#define APP_COPYRIGHT     "Copyright (C) 2026"

#endif
