#pragma once
#include "printk.h"
#include "cmdline.h"
#include "kernel.h"
#include "mc.h"
#include "term/fb/fb.h"

// This void will deinit all drivers before shutdown/reboot
// TODO: More deinit's

void do_poweroff_tasks() {
    kclear(0x000000);

    deinit_cmdline();
    deinit_fbtty();

    printk("All drivers deinited!\n");
}
