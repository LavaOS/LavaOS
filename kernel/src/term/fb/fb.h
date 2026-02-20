#pragma once
#include <../../config.h>
#include <stddef.h>
#include <stdint.h>

#define VGA_FG 0xC0C0C0
#define VGA_BG 0x000000

typedef struct Inode Inode;
typedef struct Tty Tty;
Tty* fbtty_new(Inode* keyboard, size_t framebuffer_id);
intptr_t init_fbtty(void);
void deinit_fbtty(void);
