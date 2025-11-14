#include "rootfs.h"
#include "fs/ustar/ustar.h"
#include "log.h"
#include "bootutils.h"
#include "vfs.h"
#include "print.h"
#include <minos/fcntl.h>
#include "kernel.h"

void init_rootfs(void) {
    intptr_t e = 0;
    const char* path = NULL;
    path = "/devices";
    Inode* devices;

    if((e = vfs_creat_abs(path, O_DIRECTORY, &devices)) < 0) 
        kpanic("init_rootfs: Could not create %s : %s", path, status_str(e));

    const char* initrd = "/initrd";
    const char* extras = "/extras";
    BootModule module;
    if(find_bootmodule(initrd, &module)) {
        if((e=ustar_unpack("/", module.data, module.size)) < 0) {
            kerror("Failed to unpack: %s into root: %s", initrd, status_str(e));
        }
    }
    else kpanic("Bro you can't boot live iso without initrd :|"); // Initrd not found
    if(find_bootmodule(extras, &module)) {
        if((e=ustar_unpack("/", module.data, module.size)) < 0) {
            kerror("Failed to unpack: %s into root: %s", extras, status_str(e));
        }
    }
    else kwarn("Extras not found.");
}
