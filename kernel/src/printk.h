#pragma once
#include "utils.h"
#include <stdarg.h>
#include <stdint.h>

void vprintk(const char* fmt, va_list args);
void printk(const char* fmt, ...);
void printk_set_color(uint32_t fg, uint32_t bg);
void printk_reset_color(void);
void kclear(uint32_t color);
