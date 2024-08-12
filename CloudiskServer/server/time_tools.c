#include "time_tools.h"

char* get_nowtime()
{
    time_t now;
    struct tm* tm_info;
    char* buffer = (char*)malloc(20 * sizeof(char)); // 20 字节足够

    if (buffer == NULL) {
        // 内存分配失败
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    // 获取当前时间
    time(&now);
    // 将时间转换为当地时间
    tm_info = localtime(&now);

    if (tm_info == NULL) {
        // 转换时间失败
        fprintf(stderr, "Failed to convert time\n");
        free(buffer);
        return NULL;
    }

    // 格式化时间字符串
    if (strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", tm_info) == 0) {
        // 格式化失败
        fprintf(stderr, "strftime failed\n");
        free(buffer);
        return NULL;
    }

    return buffer;
}

