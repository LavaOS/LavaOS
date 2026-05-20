#pragma once
#include "../ps2.h"
typedef struct Inode Inode;
extern Inode* ps2_keyboard_device;
void ps2_set_capslock(bool on);
bool ps2_get_capslock(void);
void ps2_set_numlock(bool on);
bool ps2_get_numlock(void);
void init_ps2_keyboard();
