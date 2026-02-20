#pragma once
#include "delay.h"
#include "printk.h"
#include "cmdline.h"

// This void will deinit all drivers before shutdown/reboot
// TODO: More deinit's
void do_poweroff_tasks() {
    kclear(0x000000);

    deinit_cmdline();

    printk("All drivers deinited!");
    delay(3);
}
