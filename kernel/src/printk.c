#include "kpanic.h"
#include "bootutils.h"
#include "framebuffer.h"
#include "fbwriter.h"
#include "serial.h"
#include "log.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

/* تنظیمات: اگر فونت ارتفاع متفاوتی داره این رو تغییر بده */
#define PRINTK_LINE_HEIGHT 16

/* موقعیت نوشتن persist می‌مونه بین فراخوانی‌ها */
static int printk_x = 0;
static int printk_y = 0;

/* helper: خروجی یک کاراکتر با استفاده از fbwriter_draw_cstr */
static void fb_put_char(FbTextWriter *w, char c) {
    char s[2] = { c, 0 };
    fbwriter_draw_cstr(w, s, 0xC0C0C0, 0x000000);
}

/* helper: تبدیل عدد بدون استفاده از توابع کتابخانه‌ای
   بنویسه (unsigned) value در مبنای base به صورت رشته و از چپ به راست چاپ می‌کنه */
static void print_padded_unsigned(FbTextWriter *w,
                                  unsigned long long val,
                                  int base,
                                  int width,
                                  bool pad_zero)
{
    char buf[65];
    int i = 0;

    if (val == 0) {
        buf[i++] = '0';
    } else {
        while (val != 0 && i < (int)sizeof(buf)-1) {
            int d = val % base;
            buf[i++] = (d < 10) ? ('0' + d) : ('a' + (d - 10));
            val /= base;
        }
    }

    int len = i;
    int pad = (width > len) ? (width - len) : 0;
    char padch = pad_zero ? '0' : ' ';

    for (int k = 0; k < pad; ++k)
        fb_put_char(w, padch);

    for (int j = len - 1; j >= 0; --j)
        fb_put_char(w, buf[j]);
}

static void print_padded_signed(FbTextWriter *w,
                                long long v,
                                int width,
                                bool pad_zero)
{
    if (v < 0) {
        fb_put_char(w, '-');
        print_padded_unsigned(w, (unsigned long long)(-v), 10, width > 0 ? width - 1 : 0, pad_zero);
    } else {
        print_padded_unsigned(w, (unsigned long long)v, 10, width, pad_zero);
    }
}

void printk(const char* fmt, ...) {
    Framebuffer fb = get_framebuffer_by_id(0);
    if (!fb.addr) return;

    FbTextWriter writer = {
        .fb = fb,
        .x = printk_x,
        .y = printk_y,
    };

    va_list args;
    va_start(args, fmt);

    const char *p = fmt;
    while (*p) {
        if (*p == '\n') {
            writer.x = 0;
            writer.y += PRINTK_LINE_HEIGHT;
            p++;
            continue;
        }

        if (*p != '%') {
            fb_put_char(&writer, *p++);
            continue;
        }

        /* % detected */
        p++;

        bool pad_zero = false;
        int width = 0;

        /* flag: 0-padding */
        if (*p == '0') {
            pad_zero = true;
            p++;
        }

        /* read width */
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }

        /* length modifier? */
        bool long_mod = false;
        bool longlong_mod = false;

        if (*p == 'l') {
            p++;
            long_mod = true;
            if (*p == 'l') {
                longlong_mod = true;
                long_mod = false;
                p++;
            }
        }

        char spec = *p++;
        switch (spec) {

            case '%':
                fb_put_char(&writer, '%');
                break;

            case 'c': {
                int c = va_arg(args, int);
                fb_put_char(&writer, (char)c);
                break;
            }

            case 's': {
                const char *s = va_arg(args, const char*);
                if (!s) s = "(null)";
                while (*s) {
                    if (*s == '\n') {
                        writer.x = 0;
                        writer.y += PRINTK_LINE_HEIGHT;
                    } else {
                        fb_put_char(&writer, *s);
                    }
                    s++;
                }
                break;
            }

            case 'd': {
                long long v;
                if (longlong_mod)      v = va_arg(args, long long);
                else if (long_mod)     v = va_arg(args, long);
                else                   v = va_arg(args, int);
                print_padded_signed(&writer, v, width, pad_zero);
                break;
            }

            case 'u': {
                unsigned long long v;
                if (longlong_mod)      v = va_arg(args, unsigned long long);
                else if (long_mod)     v = va_arg(args, unsigned long);
                else                   v = va_arg(args, unsigned int);
                print_padded_unsigned(&writer, v, 10, width, pad_zero);
                break;
            }

            case 'x': {
                unsigned long long v;
                if (longlong_mod)      v = va_arg(args, unsigned long long);
                else if (long_mod)     v = va_arg(args, unsigned long);
                else                   v = va_arg(args, unsigned int);
                print_padded_unsigned(&writer, v, 16, width, pad_zero);
                break;
            }

            default:
                /* unsupported → print literally */
                fb_put_char(&writer, '%');
                fb_put_char(&writer, spec);
                break;
        }
    }

    va_end(args);

    printk_x = writer.x;
    printk_y = writer.y;
}
