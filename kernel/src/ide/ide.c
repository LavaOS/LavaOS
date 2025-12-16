#define IDE_QUEUE_SIZE 256

#include <stdint.h>
#include "ide.h"
#include "../scheduler.h"
#include "../process.h"
#include "../memory.h"
#include "../printk.h"
#include <string.h>

uint16_t identify_buffer[256];
static ide_command_t ide_queue_storage[IDE_QUEUE_SIZE];
ide_command_t *ide_queue = ide_queue_storage;

void ide_wait_busy() {
    while (inb(IDE_STATUS) & IDE_STATUS_BSY);
}

int ide_identify(uint16_t* buffer) {
    outb(IDE_DRIVE, 0xA0);
    outb(IDE_COMMAND, IDE_CMD_IDENTIFY);

    uint8_t status = inb(IDE_STATUS);
    if (status == 0) return 0;

    ide_wait_ready();

    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(IDE_DATA);
    }
    return 1;
}

void ide_wait_ready() {
    uint8_t status;
    do {
        status = inb(IDE_STATUS);
    } while ((status & 0x80) || !(status & 0x08));
}

void ide_flush_cache() {
    outb(IDE_COMMAND, IDE_CMD_FLUSH);
    while (inb(IDE_STATUS) & 0x80);
}

void ide_read_sector(uint32_t lba, uint16_t* buffer) {
    outb(IDE_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(IDE_SECCOUNT, 1);
    outb(IDE_LBA_LOW, lba & 0xFF);
    outb(IDE_LBA_MID, (lba >> 8) & 0xFF);
    outb(IDE_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(IDE_COMMAND, IDE_CMD_READ);

    ide_wait_ready();

    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(IDE_DATA);
    }
}

void ide_write_sector(uint32_t lba, uint16_t* buffer) {
    outb(IDE_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));

    outb(IDE_SECCOUNT, 1);

    outb(IDE_LBA_LOW,  lba & 0xFF);
    outb(IDE_LBA_MID,  (lba >> 8) & 0xFF);
    outb(IDE_LBA_HIGH, (lba >> 16) & 0xFF);

    outb(IDE_COMMAND, IDE_CMD_WRITE);

    ide_wait_ready();

    for (int i = 0; i < 256; i++) {
        outw(IDE_DATA, buffer[i]);
    }

    ide_wait_busy();

    ide_flush_cache();
}

uint16_t ide_find_free(){
    for(int i = 0; i < 256; i++){
        if(!ide_queue[i].used){
            return i;
        }
    }

    return 0xFFFF;
}

uint16_t ide_find_last_node() {
    uint16_t node_index = 0;
    while (ide_queue[node_index].next_node != 0xFFFF) {
        node_index = ide_queue[node_index].next_node;
    }
    return node_index;
}

uint8_t ide_create_command(uint8_t type, uintptr_t memory_address, uint32_t disk_address, uint32_t process_index){
    uint16_t free_slot = ide_find_free();
    if(free_slot == 0xFFFF){
        return 0xFF;
    }

    ide_queue[free_slot].used = 1;
    ide_queue[free_slot].type = type;
    ide_queue[free_slot].disk_address = disk_address;
    ide_queue[free_slot].memory_address = memory_address;
    ide_queue[free_slot].process_index = process_index;
    uint16_t last_node = ide_find_last_node();
    ide_queue[last_node].next_node = free_slot;

    return free_slot;
}

void ide_process_next(){
    if(ide_queue[0].next_node == 0xFFFF){
        return;
    }

    if(ide_queue[ide_queue[0].next_node].type){
        ide_write_sector(ide_queue[ide_queue[0].next_node].disk_address, (uint16_t*)(uintptr_t)ide_queue[ide_queue[0].next_node].memory_address);
    }else{
        ide_read_sector(ide_queue[ide_queue[0].next_node].disk_address, (uint16_t*)(uintptr_t)ide_queue[ide_queue[0].next_node].memory_address);
    }
    
    if(ide_queue[ide_queue[0].next_node].process_index != 0){
        resume_process(ide_queue[ide_queue[0].next_node].process_index);
    }
    ide_queue[0].next_node = ide_queue[ide_queue[0].next_node].next_node;
}

void ide_init(){
    uint8_t status = inb(IDE_STATUS);
    printk("IDE STATUS = %x\n", status);
    if (!ide_identify(identify_buffer)) {
        printk("IDE Identify failed\n");
        return;
    }

    ide_queue[0].used = 1;
    ide_queue[0].next_node = 0xFFFF;

    printk("IDE initialized\n");
}
