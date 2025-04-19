/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: formatted printing (e.g., printf)
 * format_to_str() converts a format to a C string,
 * e.g., converts ("%s-%d", "egos", 2000) to "egos-2000"
 * term_write() prints a C string to the screen
 */

#include "egos.h"
#include "servers.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

void ull_to_str(unsigned long long num, char *str) {
    int i = 0;
    if (num == 0) {
        str[i++] = '0';
    } else {
        while (num != 0) {
            str[i++] = (num % 10) + '0'; // 取最后一位转为字符
            num /= 10; // 移除最后一位
        }
    }
    str[i] = '\0'; // 终止符

    // 反转字符串（因为数字是逆序存储的）
    for (int j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }
}

unsigned int format_to_str_len(const char *fmt,va_list args){
    unsigned int ret = 0;
    for(; *fmt != 0; fmt++){
        if(*fmt != '%'){
            ret += 1;
        }else{
            fmt++;
            if(*fmt == 'c'){
                ret += 1;
                va_arg(args, int);
            }else if(*fmt == 'd' || *fmt == 'u' || *fmt == 'p' || *fmt == 'x'){
                ret += 32;
                va_arg(args, int);
            }else if(*fmt == 'l' && *(fmt+1) == 'l' && *(fmt+2) == 'u'){
                fmt += 2;
                ret += 32;
                va_arg(args, unsigned long long);
            }else if(*fmt == 's'){
                ret += strlen(va_arg(args, char *));
            }
        }
    }
    return ret;
}


void format_to_str(char* out, const char* fmt, va_list args) {
    for(out[0] = 0; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            strncat(out, fmt, 1);
        } else {
            fmt++;
            if (*fmt == 's') {
                strcat(out, va_arg(args, char*));
            } else if (*fmt == 'd') {
                itoa(va_arg(args, int), out + strlen(out), 10);
            } else if(*fmt == 'c'){
                int tmp = strlen(out);
                // 必须四字节对其，否则会出现异常
                out[tmp] = (char)va_arg(args, int);
                out[tmp + 1] = '\0';
            } else if(*fmt == 'u'){
                utoa(va_arg(args, unsigned int), out + strlen(out), 10);
            } else if(*fmt == 'p'){
                utoa(va_arg(args, int), out + strlen(out), 10);
            } else if(*fmt == 'l' && *(fmt+1) == 'l' && *(fmt+2) == 'u'){
                fmt += 2; 
                char llu[21];
                ull_to_str(va_arg(args, unsigned long long), llu);
                strcat(out, llu);
            } else if(*fmt == 'x'){
                utoa(va_arg(args, int), out + strlen(out), 16);
            }
        }
    }
}


#define LOG(prefix, suffix)                                                    \
    char buf[512];                                                             \
    strcpy(buf, prefix);                                                       \
    va_list args;                                                              \
    va_start(args, format);                                                    \
    format_to_str(buf + strlen(prefix), format, args);                         \
    va_end(args);                                                              \
    strcat(buf, suffix);                                                       \
    term_write(buf, strlen(buf));

int my_printf(const char* format, ...) { LOG("", ""); }

int INFO(const char* format, ...) { LOG("[INFO] ", "\r\n") }

int FATAL(const char* format, ...) {
    LOG("\x1B[1;31m[FATAL] ", "\x1B[1;0m\r\n") /* red color */
    while (1);
}

int SUCCESS(const char* format, ...) {
    LOG("\x1B[1;32m[SUCCESS] ", "\x1B[1;0m\r\n") /* green color */
}

int CRITICAL(const char* format, ...) {
    LOG("\x1B[1;33m[CRITICAL] ", "\x1B[1;0m\r\n") /* yellow color */
}
