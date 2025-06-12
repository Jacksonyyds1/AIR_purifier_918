// unified_logger.h

#if 0
#ifndef UNIFIED_LOGGER_H
#define UNIFIED_LOGGER_H

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// ��־������
typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN  = 1,
    LOG_LEVEL_INFO  = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_VERBOSE = 4
} log_level_t;

// ȫ����־�������
extern log_level_t g_current_log_level;

// ��־���ؿ���
#ifndef LOG_ENABLE
#define LOG_ENABLE 1  // Ĭ��������־
#endif

#if LOG_ENABLE

// ������־��ӡ����
void log_printf(log_level_t level, const char* tag, const char* file, 
                int line, const char* func, const char* fmt, ...);

// ��ȡ�ļ�����ȥ��·����
static inline const char* get_basename(const char* path) {
    const char* base = path;
    const char* p = path;
    while (*p) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
        p++;
    }
    return base;
}

// ��־�궨�� - ��������Ϣ
#define LOG_PRINT(level, tag, fmt, ...) \
    do { \
        if (level <= g_current_log_level) { \
            log_printf(level, tag, get_basename(__FILE__), __LINE__, __func__, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

// �򻯵���־��
#define LOGE(fmt, ...) LOG_PRINT(LOG_LEVEL_ERROR,   "[E]", fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG_PRINT(LOG_LEVEL_WARN,    "[W]", fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG_PRINT(LOG_LEVEL_INFO,    "[I]", fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) LOG_PRINT(LOG_LEVEL_DEBUG,   "[D]", fmt, ##__VA_ARGS__)
#define LOGV(fmt, ...) LOG_PRINT(LOG_LEVEL_VERBOSE, "[V]", fmt, ##__VA_ARGS__)

// ������־��
#define LOGE_IF(cond, fmt, ...) do { if (cond) LOGE(fmt, ##__VA_ARGS__); } while(0)
#define LOGW_IF(cond, fmt, ...) do { if (cond) LOGW(fmt, ##__VA_ARGS__); } while(0)
#define LOGI_IF(cond, fmt, ...) do { if (cond) LOGI(fmt, ##__VA_ARGS__); } while(0)
#define LOGD_IF(cond, fmt, ...) do { if (cond) LOGD(fmt, ##__VA_ARGS__); } while(0)
#define LOGV_IF(cond, fmt, ...) do { if (cond) LOGV(fmt, ##__VA_ARGS__); } while(0)

// ʮ���������ݴ�ӡ
void log_hex_dump(log_level_t level, const char* tag, const void* data, size_t len);
#define LOG_HEX(level, tag, data, len) log_hex_dump(level, tag, data, len)

#else  // LOG_ENABLE == 0

// ����������־��
#define LOG_PRINT(level, tag, fmt, ...)
#define LOGE(fmt, ...)
#define LOGW(fmt, ...)
#define LOGI(fmt, ...)
#define LOGD(fmt, ...)
#define LOGV(fmt, ...)
#define LOGE_IF(cond, fmt, ...)
#define LOGW_IF(cond, fmt, ...)
#define LOGI_IF(cond, fmt, ...)
#define LOGD_IF(cond, fmt, ...)
#define LOGV_IF(cond, fmt, ...)
#define LOG_HEX(level, tag, data, len)

#endif // LOG_ENABLE

// ��־ϵͳ���ƺ���
void log_init(log_level_t level);
void log_set_level(log_level_t level);
log_level_t log_get_level(void);
const char* log_level_to_string(log_level_t level);

#ifdef __cplusplus
}
#endif

#endif // UNIFIED_LOGGER_H

// ================================================================
// unified_logger.c
#include "unified_logger.h"
#include <string.h>
#include <time.h>

// ȫ����־������ͳ�ʼ��
log_level_t g_current_log_level = LOG_LEVEL_INFO;

#if LOG_ENABLE

// ��־�����ַ���
static const char* level_strings[] = {
    "ERROR", "WARN", "INFO", "DEBUG", "VERBOSE"
};

// ��ȡʱ����ַ���
static void get_timestamp_string(char* buffer, size_t size) {
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    // C99�����ϰ汾��֧��time����
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    if (tm_info) {
        strftime(buffer, size, "%H:%M:%S", tm_info);
    } else {
        snprintf(buffer, size, "??:??:??");
    }
#else
    // ���ڲ�֧��time������Ƕ��ʽϵͳ������ʹ��ϵͳtick
    // �����ṩһ���򵥵�ʵ��
    static unsigned long tick_count = 0;
    tick_count++;
    snprintf(buffer, size, "%08lu", tick_count);
#endif
}

// ��Ҫ����־��ӡ����
void log_printf(log_level_t level, const char* tag, const char* file, 
                int line, const char* func, const char* fmt, ...) {
    
    if (level > g_current_log_level) {
        return;
    }
    
    char timestamp[32];
    get_timestamp_string(timestamp, sizeof(timestamp));
    
    // ��ӡ��־ͷ����Ϣ
    printf("[%s] %s %s %s:%d %s() ", 
           timestamp, 
           level_strings[level], 
           tag, 
           file, 
           line, 
           func);
    
    // ��ӡ�û���ʽ��������
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);  // ȷ���������
}

// ʮ���������ݴ�ӡ
void log_hex_dump(log_level_t level, const char* tag, const void* data, size_t len) {
    if (level > g_current_log_level || !data || len == 0) {
        return;
    }
    
    const unsigned char* bytes = (const unsigned char*)data;
    char timestamp[32];
    get_timestamp_string(timestamp, sizeof(timestamp));
    
    printf("[%s] %s %s HEX DUMP (%zu bytes):\n", 
           timestamp, level_strings[level], tag, len);
    
    for (size_t i = 0; i < len; i += 16) {
        printf("  %04X: ", (unsigned int)i);
        
        // ��ӡʮ������
        for (size_t j = 0; j < 16; j++) {
            if (i + j < len) {
                printf("%02X ", bytes[i + j]);
            } else {
                printf("   ");
            }
        }
        
        printf(" ");
        
        // ��ӡASCII�ַ�
        for (size_t j = 0; j < 16 && i + j < len; j++) {
            unsigned char c = bytes[i + j];
            printf("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        
        printf("\n");
    }
    fflush(stdout);
}

#endif // LOG_ENABLE

// ��־ϵͳ��ʼ��
void log_init(log_level_t level) {
    g_current_log_level = level;
    LOGI("Log system initialized, level: %s", log_level_to_string(level));
}

// ������־����
void log_set_level(log_level_t level) {
    g_current_log_level = level;
    LOGI("Log level changed to: %s", log_level_to_string(level));
}

// ��ȡ��ǰ��־����
log_level_t log_get_level(void) {
    return g_current_log_level;
}

// ��־����ת�ַ���
const char* log_level_to_string(log_level_t level) {
#if LOG_ENABLE
    if (level < sizeof(level_strings) / sizeof(level_strings[0])) {
        return level_strings[level];
    }
#endif
    return "UNKNOWN";
}

// ================================================================
// ʹ��ʾ��
/*
#include "unified_logger.h"

int main() {
    // ��ʼ����־ϵͳ
    log_init(LOG_LEVEL_DEBUG);
    
    // ������־����
    LOGE("This is an error message: %d", 123);
    LOGW("This is a warning message: %s", "test");
    LOGI("This is an info message");
    LOGD("This is a debug message: %f", 3.14);
    LOGV("This is a verbose message");
    
    // ������־����
    int error_code = 0;
    LOGE_IF(error_code != 0, "Error occurred: %d", error_code);
    
    // ʮ���������ݴ�ӡ
    unsigned char data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    LOG_HEX(LOG_LEVEL_INFO, "[DATA]", data, sizeof(data));
    
    // ��̬������־����
    log_set_level(LOG_LEVEL_ERROR);
    LOGI("This message will not be printed");
    LOGE("But this error will be printed");
    
    return 0;
}
*/

#endif