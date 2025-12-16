#pragma once
#define IDE_DATA        0x1F0
#define IDE_ERROR       0x1F1
#define IDE_SECCOUNT    0x1F2
#define IDE_LBA_LOW     0x1F3
#define IDE_LBA_MID     0x1F4
#define IDE_LBA_HIGH    0x1F5
#define IDE_DRIVE       0x1F6
#define IDE_COMMAND     0x1F7
#define IDE_STATUS      0x1F7

#define IDE_CMD_IDENTIFY 0xEC
#define IDE_CMD_FLUSH    0xE7
#define IDE_CMD_WRITE    0x30
#define IDE_CMD_READ     0x20

#define IDE_STATUS_BSY   0x80
#define IDE_STATUS_DRQ   0x08

#include <stdint.h>
#include "../port.h"

typedef struct
{
    uint8_t type; // 0=read 1=write
    uint32_t memory_address;
    uint32_t disk_address;
    uint8_t used;
    uint32_t process_index;
    uint16_t next_node;
} ide_command_t;

extern uint16_t identify_buffer[256];

void ide_init();
void ide_wait_ready();
int ide_identify(uint16_t* buffer);
