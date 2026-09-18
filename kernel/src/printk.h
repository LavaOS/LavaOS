#pragma once
#include "utils.h"
#include <stdarg.h>
#include <stdint.h>

void vprintk(const char* fmt, va_list args);
void printk(const char* fmt, ...);
void printk_set_color(uint32_t fg, uint32_t bg);
void printk_reset_color(void);
void printl_ok(const char* fmt, ...);
void printl_wait(const char* fmt, ...);
void printl_verb(const char* fmt, ...);
void printl_fail(const char* fmt, ...);
void kclear(uint32_t color);
