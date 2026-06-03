#include "ustar.h"
#include "../../printk.h"
#include <fileutils.h>
#include <log.h>
#include <minos/fcntl.h>
int octtoi(const char* str, size_t len) {
    int res = 0;
    while (len > 0) {
        if(str[0] < '0' || str[0] > '8') return -1;
        res = res * 8 + str[0]-'0';
        str++;
        len--;
    }
    return res;
}

typedef char Ustar12[12];
typedef struct {
    char filename[1024];
    uint64_t mode;
    uint64_t ownerid;
    uint64_t groupid;
    Ustar12 filesize;
    Ustar12 modtime;
    uint64_t checksum; 
    uint8_t type;
    char linkedf_name[1024];
    char ustar_indict[6];
    char ustar_version[2];
    char owner_name[32];
    char group_name[32];
    uint64_t majornum;
    uint64_t minornum;
    char filename_prefix[155];
} UstarMeta;


#define USTAR_FILESIZE_OFF 124
#define USTAR_TYPE_OFF  156
#define USTAR_MAGIC_OFF 257
#define USTAR_DATA 512
intptr_t ustar_unpack(const char* into, const char* ustar_data, size_t ustar_size) {
    const char* ustar_end = ustar_data + ustar_size;
    intptr_t e;
    size_t into_len = strlen(into);
    
    if(into_len >= PATH_MAX) return -LIMITS;
    
    // Use stack allocation instead of malloc (safe for real HW)
    char path[PATH_MAX];
    
    while(ustar_data < ustar_end) {
        // Check if we have enough data for header (512 bytes)
        if(ustar_end - ustar_data < 512) {
            printk("[USTR] Not enough data for header\n");
            return -BUFFER_TOO_SMALL;
        }
        
        // Check magic
        if(memcmp(ustar_data + USTAR_MAGIC_OFF, "ustar", 5) != 0) {
            // Not a ustar entry - probably end of archive
            break;
        }
        
        int size = octtoi(ustar_data + USTAR_FILESIZE_OFF, 11);
        if(size < 0) {
            printk("[USTR] Invalid size in ustar header\n");
            return -INVALID_PARAM;
        }
        
        // Check if we have enough data for file content
        if(ustar_end - ustar_data < 512 + size) {
            printk("[USTR] Not enough data for file content (size=%d)\n", size);
            return -BUFFER_TOO_SMALL;
        }
        
        uint8_t type = *(((uint8_t*)ustar_data) + USTAR_TYPE_OFF);
        size_t name_len = 0;
        const char* name = ustar_data;
        
        if(memcmp(name, "./", 2) == 0) name += 2;
        
        while(name[name_len] && name_len < 98) {
            name_len++;
        }
        
        if(name_len == 98) return -INVALID_PATH;
        if(name_len > 0 && name[name_len - 1] == '/') name_len--;
        if(into_len + name_len >= PATH_MAX) return -LIMITS;
        
        memcpy(path, into, into_len);
        memcpy(path + into_len, name, name_len);
        path[into_len + name_len] = '\0';
        
        ktrace("[USTR] %s of type %c size %zu", path, type, size);
        
        if(type == '5') {
            Inode* dir = NULL;
            if((e = vfs_creat_abs(path, O_DIRECTORY, &dir)) < 0) {
                if(e != -ALREADY_EXISTS) {
                    printk("[USTR] Could not mkdir %s : %s\n", path, status_str(e));
                    return e;
                }
            }
            if(dir) idrop(dir);
        } else {
            Inode* file;
            if((e = vfs_creat_abs(path, 0, &file)) < 0) {
                printk("[USTR] Could not create %s : %s\n", path, status_str(e));
                return e;
            }
            if((e = write_exact(file, ustar_data + 512, size, 0)) < 0) {
                printk("[USTR] Could not write data: %s : %s\n", path, status_str(e));
                idrop(file);
                return e;
            }
            if(file) idrop(file);
        }
        
        // Advance to next entry
        size_t blocks = ((size + 511) / 512) + 1;
        ustar_data += blocks * 512;
    }
    
    return 0;
}
