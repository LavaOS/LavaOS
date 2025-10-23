#include "kpanic.h"
#include "bootutils.h"
#include "framebuffer.h"
#include "fbwriter.h"
#include "serial.h"
#include "log.h"
#include <stdarg.h>
#include "print_base.h"

void printk(const char* fmt, ...) {
    Framebuffer fb = get_framebuffer_by_id(0);
    if(fb.addr) {
        FbTextWriter writer = {
            .fb = fb,
            .x = 0,
            .y = 0,
        };
        fbwriter_draw_cstr(&writer, fmt, 0xFFFFFF, 0x000000);
    }
}
