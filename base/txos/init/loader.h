#ifndef LDR_H
#define LDR_H

#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include "../ke/string/string.h"
#include "../ke/base.h"
#include "../ke/memory/heap/kheap.h"
#include "../ke/memory/paging/paging.h"
#include "../ke/memory/memory.h"
#include "../ke/disk/disk.h"
#include "../ke/fs/pparser.h"
#include "../ke/disk/streamer.h"
#include "../ke/fs/fat/fat16.h"

#define IMAGE_NT_SIGNATURE 0x00004550
#define IMAGE_FILE_DLL 0x2000
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10B
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20B
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC 0x107
#define IMAGE_NT_OPTIONAL_HDR_MAGIC IMAGE_NT_OPTIONAL_HDR32_MAGIC
#define IMAGE_DIRECTORY_ENTRY_TLS 9
#define IMAGE_DIRECTORY_ENTRY_BASERELOC 5
#define IMAGE_DIRECTORY_ENTRY_IMPORT 1
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION 3
#define IMAGE_DIRECTORY_ENTRY_EXPORT 0
#define IMAGE_FILE_EXECUTABLE_IMAGE 0x0002
#define IMAGE_REL_BASED_ABSOLUTE 0
#define IMAGE_REL_BASED_HIGH 1
#define IMAGE_REL_BASED_LOW 2
#define IMAGE_REL_BASED_HIGHLOW 3
#define IMAGE_REL_BASED_DIR64 10
#define IMAGE_REL_BASED_DIR32 6

#define HIWORD(l) ((WORD)((DWORD)(l) >> 16))
#define LOWORD(l) ((WORD)((DWORD)(l) & 0xFFFF))
#define ADD_OFFSET_TO_POINTER(ptr, offset) ((void*)((BYTE*)(ptr) + (offset)))

typedef unsigned long DWORD64;
typedef DWORD64 DWORD_PTR;
typedef DWORD64* PDWORD64;

LPVOID LdrLoadPe(const LPSTR path);
NTSTATUS LdrExecBat(const char *path);

#endif
