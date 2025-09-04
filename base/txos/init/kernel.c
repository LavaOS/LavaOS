/*++

Free95 20x/TX Kernel

You may only use this code if you agree to the terms of the Free95 Source Code License agreement (GNU GPL v3) (see LICENSE).
If you do not agree to the terms, do not use the code.


Module Name:

    kernel.c

Abstract:

    This module implements the kernel initialization code.

--*/

#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include "../ke/string/string.h"
#include "../ke/idt/idt.h"
#include "../ke/io/io.h"
#include "../ke/memory/heap/kheap.h"
#include "../ke/memory/paging/paging.h"
#include "../ke/memory/memory.h"
#include "../ke/disk/disk.h"
#include "../ke/fs/pparser.h"
#include "../ke/disk/streamer.h"
#include "../ke/fs/fat/fat16.h"
#include "../ke/graphics/graphics.h"
#include "../ke/gdt/gdt.h"
#include "../ke/config.h"
#include "../ke/task/tss.h"
#include "../ke/base.h"
#include "../ke/ntdll.h"
#include "loader.h"

static char copyright[] =
    "(C) Copyright 2025 Versoft Free95.\n"
    "This is free software and comes with ABSOLUTELY NO"
    " WARRANTY;\nyou can redistribute it and/or modify it under the terms of the\n"
    "GNU General Public License Version 3. "
    "The full license document is in LICENSE\nin the root directory of the source code.\n"
    "If you do not agree to the terms, do not use the source code nor the product.\n";

uint16_t* video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;

/* Hardware text mode color constants. */
enum vga_color
{
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

uint16_t terminal_make_char(char c, char colour)
{
    return (colour << 8) | c;
}

void terminal_putchar(int x, int y, char c, char colour)
{
    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, colour);
}

void terminal_scroll() {
    // Move each line up by one row
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            video_mem[(y - 1) * VGA_WIDTH + x] = video_mem[y * VGA_WIDTH + x];
        }
    }
    // Clear the last row
    for (int x = 0; x < VGA_WIDTH; x++) {
        video_mem[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = terminal_make_char(' ', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    }
}

void terminal_writechar(char c, char colour) {
    if (c == '\n') {
        terminal_col = 0;
        terminal_row++;
    } else {
        terminal_putchar(terminal_col, terminal_row, c, colour);
        terminal_col++;
        if (terminal_col >= VGA_WIDTH) {
            terminal_col = 0;
            terminal_row++;
        }
    }

    if (terminal_row >= VGA_HEIGHT) {
        terminal_scroll();
        terminal_row = VGA_HEIGHT - 1;
    }
}

void terminal_initialize()
{
    video_mem = (uint16_t*)(0xB8000);
    terminal_row = 0;
    terminal_col = 0;
    for (int y = 0; y < VGA_HEIGHT + 10; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, y, ' ', vga_entry_color(VGA_COLOR_BLUE, VGA_COLOR_BLUE));
        }
    }
}

void FirstTimeBootInit()
{
	video_mem = (uint16_t*)(0xB8000);
    terminal_row = 0;
    terminal_col = 0;
    for (int y = 0; y < VGA_HEIGHT + 10; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, y, ' ', vga_entry_color(VGA_COLOR_BLUE, VGA_COLOR_BLUE));
        }
    }   
}

void print(const char* str)
{
    size_t len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        terminal_writechar(str[i], vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    }
}

static int DbgInit()
{
   outb(PORT + 1, 0x00);    // Disable all interrupts
   outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb(PORT + 1, 0x00);    //                  (hi byte)
   outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
   outb(PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
   outb(PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

   // Check if serial is faulty
   if(insb(PORT + 0) != 0xAE)
   {
      return 1;
   }

   // If serial is not faulty set it in normal operation mode
   // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
   outb(PORT + 4, 0x0F);
   return 0;
}

int IoTransEmpty()
{
   return insb(PORT + 5) & 0x20;
}

void DbgPutc(char a)
{
   while (IoTransEmpty() == 0);

   outb(PORT, a);
}

#define MAX_DBGPRINT_BUFFER 1024

void DbgPrint(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    while (*format)
    {
        if (*format == '%') // Check for format specifier
        {
            format++;
            switch (*format)
            {
                case 'd': // Decimal integer
                {
                    int value = va_arg(args, int);
                    if (value < 0)
                    {
                        DbgPutc('-');
                        value = -value;
                    }
                    char buffer[12]; // Enough for 32-bit integer
                    int i = 0;
                    do
                    {
                        buffer[i++] = '0' + (value % 10);
                        value /= 10;
                    } while (value > 0);
                    while (--i >= 0)
                    {
                        DbgPutc(buffer[i]);
                    }
                    break;
                }
                case 'x': // Hexadecimal
                {
                    unsigned int value = va_arg(args, unsigned int);
                    char hex_digits[] = "0123456789abcdef";
                    char buffer[9]; // Enough for 32-bit hex value
                    int i = 0;
                    do
                    {
                        buffer[i++] = hex_digits[value % 16];
                        value /= 16;
                    } while (value > 0);
                    while (--i >= 0)
                    {
                        DbgPutc(buffer[i]);
                    }
                    break;
                }
                case 's': // String
                {
                    const char *str = va_arg(args, const char *);
                    while (*str)
                    {
                        DbgPutc(*str++);
                    }
                    break;
                }
                case '%': // Literal '%'
                    DbgPutc('%');
                    break;
                default: // Unsupported format specifier
                    DbgPutc('%');
                    DbgPutc(*format);
                    break;
            }
        }
        else
        {
            DbgPutc(*format); // Regular character
        }
        format++;
    }

    va_end(args);
}


int IoSerialReceive()
{
   return insb(PORT + 5) & 1;
}

char DbgGetc()
{
   while (IoSerialReceive() == 0);

   return insb(PORT);
}

char *strstr(const char *haystack, const char *needle)
{
    if (!*needle) return (char *)haystack;

    const char *h, *n;
    while (*haystack) {
        h = haystack;
        n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) return (char *)haystack; // Found needle
        haystack++;
    }
    return NULL; // Not found
}

int fetchwh(const char *str, int *width, int *height)
{
    *width = 0;
    *height = 0;
    const char *p = str;

    // Parse width
    while (*p >= '0' && *p <= '9') {
        *width = *width * 10 + (*p - '0');
        p++;
    }
    if (*p != 'x') return 0; // Invalid format
    p++;

    // Parse height
    while (*p >= '0' && *p <= '9') {
        *height = *height * 10 + (*p - '0');
        p++;
    }
    return (*p == '\0' || *p == '\n') ? 2 : 0; // Success if end of string
}

static struct paging_4gb_chunk* kernel_chunk = 0;

struct tss tss;
struct gdt gdt_real[FREE95_TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured[FREE95_TOTAL_GDT_SEGMENTS] =
{
    {.base = 0x00, .limit = 0x00, .type = 0x00},                // NULL Segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x9a},           // Kernel code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x92},            // Kernel data segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf8},              // User code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf2},             // User data segment
    {.base = (uint32_t)&tss, .limit=sizeof(tss), .type = 0xE9}      // TSS Segment
};

extern void jump_usermode();

void InitializeUnicodeString(PUNICODE_STRING UnicodeString, const CHAR* SourceString)
{
    UnicodeString->Buffer = (PCHAR)SourceString;
    UnicodeString->Length = (UCHAR)(strlen(SourceString) * sizeof(CHAR));
    UnicodeString->MaximumLength = UnicodeString->Length + sizeof(CHAR);
}

void PrintString(const char *msg)
{
    UNICODE_STRING stri;
    InitializeUnicodeString(&stri, msg);
    NtDisplayString(&stri);
}

void (*dll_start)();

#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH 2

char output[128] = {0};

int contains_newline(const char *str)
{
    if (str == NULL) // Check for null pointer
    {
        return 0;
    }

    while (*str != '\0') // Iterate through the string
    {
        if (*str == '\n') // Check for newline character
        {
            return 1;
        }
        str++;
    }
    return 0; // Return false if no newline character found
}

static char ex_buffer[256];
int exec = 0;

void SetExecBuffer(char *b)
{
    strcpy(ex_buffer, b);
    exec = 1;
}

uint32_t *buffer = 0;
uint32_t *fb = 0;

int w = 640;
int h = 480;

void RenderChar(uint32_t x, uint32_t y, uint32_t color, uint32_t *framebuffer,
                char c, uint32_t screen_width);
void RenderString(uint32_t x, uint32_t y, uint32_t color, uint32_t *framebuffer,
                  const char *str, uint32_t screen_width);

static uint32_t global_cursor_x = 0;
static uint32_t global_cursor_y = 0;

void PrintChar(char str)
{
    if (str == '\b')
    {
        if (global_cursor_x >= 8)
        {
            global_cursor_x -= 8;
        }
        else if (global_cursor_y >= 16)
        {
            global_cursor_y -= 16;
            global_cursor_x = w - 8;
        }

        for (uint8_t row = 0; row < 16; row++)
        {
            for (uint8_t col = 0; col < 8; col++)
            {
                PutPixel(global_cursor_x + col, global_cursor_y + row, 0xFF0000FF, buffer);
            }
        }

        return;
    }

    RenderChar(global_cursor_x, global_cursor_y, 0xFFFFFFFF, buffer, str, w);

    if (str == '\n') // Handle newline
    {
        global_cursor_x = 0;
        global_cursor_y += 16;
    }
    else
    {
        global_cursor_x += 8;
        if (global_cursor_x + 8 > w) // Wrap around
        {
            global_cursor_x = 0;
            global_cursor_y += 16;
        }
    }
}

void Print(const char *str)
{
    RenderString(global_cursor_x, global_cursor_y, 0xFFFFFFFF, buffer, str, w);

    // Update global cursor position
    while (*str) {
        if (*str == '\n') {
            global_cursor_x = 0;
            global_cursor_y += 16;
        } else {
            global_cursor_x += 8;
            if (global_cursor_x + 8 > w) {
                global_cursor_x = 0;
                global_cursor_y += 16;
            }
        }
        str++;
    }
}

static char HelpMsg[] =
    "\nList of commands\n"
    "help - Display this message\n"
    "cls - Clear the screen\n"
    "If you do not see a command on this list, it is treated as an executable or batch script.\n";

void ClearScreen()
{
    FillRectangle(0, 0, w, h, 0xFF0000FF, buffer);
}

void NsExec(char *ex_buffer, int isBat)
{
    if (strcmp(ex_buffer, "cls") == 0)
    {
        memset(buffer, 0, w  * h * 32 / 8);
        ClearScreen();
        global_cursor_x = 0;
        global_cursor_y = 0;

        exec = 0;
    }
    else if (strcmp(ex_buffer, "help") == 0)
    {
        PrintString(HelpMsg);

        exec = 0;
    }
    else if (ex_buffer[0] != '\0')
    {
        PrintString("\n");

        if (!isBat)
        {
            LPVOID syscallResult;

            asm volatile (
                    "movl $0x02, %%eax\n\t"
                    "movl %0, %%ebx\n\t"
                    "int $0x2e\n\t"
                    "mov %%eax, %0\n"
                    : "=r" (syscallResult)
                    : "r"(ex_buffer)
                    : "%eax", "%ebx"
            );

            typedef WINBOOL(*MAIN)(DWORD, PCHAR);

            if (syscallResult != NULL)
            {
                ((MAIN)syscallResult)(1, NULL);
            }
            else
            {
                PrintString("'");
                PrintString(ex_buffer);
                PrintString("' is not recognized as an internal or external command, operable program or batch file.\n");
            }
        }
        else
        {
            NTSTATUS syscallResult;

            asm volatile (
                    "movl $0x04, %%eax\n\t"
                    "movl %0, %%ebx\n\t"
                    "int $0x2e\n\t"
                    "mov %%eax, %0\n"
                    : "=r" (syscallResult)
                    : "r"(ex_buffer)
                    : "%eax", "%ebx"
            );

            if (syscallResult == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                PrintString("'");
                PrintString(ex_buffer);
                PrintString("' is not recognized as an internal or external command, operable program or batch file.\n");
            }
        }

        exec = 0;
    }
    else if (ex_buffer[0] == '\0')
    {
        PrintString("\n");
        exec = 0;
    }

    PrintString("0:/> ");
}

int IsBatFile(const char *buffer)
{
    uint32_t len = strlen(buffer);
    if (len < 4)
    {
        return 0;
    }

    return strcmp(buffer + len - 4, ".bat") == 0;
}

void KiUserInit()
{
    memset(buffer, 0, w  * h * 32 / 8);

    FillRectangle(0, 0, w, h, 0xFF0000FF, buffer);

    PrintString(copyright);

    PrintString("\nFree95 Native Shell\n(C) Versoft Free95 Native Shell Team 2025\n\nCheck out SynthLauncher!\n\n");

    PrintString("0:/> ");

    while(1)
    {
        if (exec)
        {
            if(IsBatFile(ex_buffer))
            {
                NsExec(ex_buffer, 1);
            }
            else
            {
                NsExec(ex_buffer, 0);
            }
        }

        memcpy(fb, buffer, w  * h * 32 / 8);
    }
}

// Warning: PE structures ahead. Close your eyes or bleach them!
#pragma pack(push, 1)
typedef struct {
    uint16_t e_magic;      // Magic number ("MZ")
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    uint32_t e_lfanew;     // Offset to PE header
} DOSHeader;

typedef struct {
    uint32_t Signature;    // PE signature ("PE\0\0")
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} PEHeader;

typedef struct _IMAGE_DATA_DIRECTORY
{
    DWORD VirtualAddress;		// RVA of table
    DWORD Size;			// size of table
} DataDirectory, *PIMAGE_DATA_DIRECTORY;

typedef struct {
    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;
    uint32_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    DataDirectory DataDirectory[16];
} OptionalHeader;

typedef struct {
    char Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} SectionHeader;

typedef struct _IMAGE_EXPORT_DIRECTORY {
  uint32_t Characteristics;
  uint32_t TimeDateStamp;
  uint16_t MajorVersion;
  uint16_t MinorVersion;
  uint32_t Name;
  uint32_t Base;
  uint32_t NumberOfFunctions;
  uint32_t NumberOfNames;
  uint32_t AddressOfFunctions;
  uint32_t AddressOfNames;
  uint16_t AddressOfNameOrdinal;
} ExportDirectory,*PIMAGE_EXPORT_DIRECTORY;

#pragma pack(pop)

void snprintf(char *buffer, size_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    size_t index = 0;
    while (*format && index < size - 1) {
        if (*format == '%' && *(format + 1) == 's') {
            // Handle %s placeholder
            format += 2; // Skip %s
            const char *arg = va_arg(args, const char *);
            while (*arg && index < size - 1) {
                buffer[index++] = *arg++;
            }
        } else {
            // Copy regular characters
            buffer[index++] = *format++;
        }
    }

    buffer[index] = '\0'; // Null-terminate the buffer
    va_end(args);
}

/* Author: ArefProgrammer */
void get_cpu_info()
{
    uint32_t eax, ebx, ecx, edx;
    char cpu_brand[49];

    // Get CPU brand string part 1
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x80000002)
    );
    memcpy(cpu_brand, &eax, 4);
    memcpy(cpu_brand + 4, &ebx, 4);
    memcpy(cpu_brand + 8, &ecx, 4);
    memcpy(cpu_brand + 12, &edx, 4);

    // Get CPU brand string part 2
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x80000003)
    );
    memcpy(cpu_brand + 16, &eax, 4);
    memcpy(cpu_brand + 20, &ebx, 4);
    memcpy(cpu_brand + 24, &ecx, 4);
    memcpy(cpu_brand + 28, &edx, 4);

    // Get CPU brand string part 3
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x80000004)
    );
    memcpy(cpu_brand + 32, &eax, 4);
    memcpy(cpu_brand + 36, &ebx, 4);
    memcpy(cpu_brand + 40, &ecx, 4);
    memcpy(cpu_brand + 44, &edx, 4);

    cpu_brand[48] = '\0';
    print(cpu_brand); // Display the CPU model information
}

void DbgLog(const char *msg, int type)
{
    if (type == LOG_SUCCESS)
        DbgPrint("[SUCCESS] ");
    else if (type == LOG_FAIL)
        DbgPrint("[FAIL] ");
    else if (type == LOG_ERROR)
        DbgPrint("[ERROR] ");
    else if (type == LOG_WARN)
        DbgPrint("[WARNING] ");
    else if (type == LOG_INFO)
        DbgPrint("[INFORMATION] ");

    DbgPrint(msg);
    DbgPrint("\n");
}

char fontdata[5900];

void LoadPsfFont(const char *path)
{
    int fd = fopen(path, "r");

	if (fd)
	{
		fread(fontdata, 5900, 1, fd);
    }
    else
    {
        DbgLog("Failed to load PSF Font", LOG_ERROR);
    }
}

typedef struct {
    uint8_t magic[2];     // Magic bytes (0x36, 0x04)
    uint8_t mode;         // Font mode
    uint8_t charsize;     // Character size in bytes
} PSF1_Header;

void RenderChar(uint32_t x, uint32_t y, uint32_t color, uint32_t *framebuffer,
                char c, uint32_t screen_width)
{
    // Get the PSF1 header from the font data
    const PSF1_Header *header = (const PSF1_Header *)fontdata;

    // Validate the magic bytes
    if (header->magic[0] != 0x36 || header->magic[1] != 0x04) {
        return; // Invalid PSF1 font
    }

    // Get the character size and glyph data
    uint8_t charsize = header->charsize;
    uint8_t *glyph = (uint8_t*)fontdata + sizeof(PSF1_Header) + (c * charsize);

    // Render the character
    for (uint8_t row = 0; row < 16; row++) { // PSF1 typically uses an 8x8 or 8x16 grid
        uint8_t line = glyph[row]; // Get the current row of the glyph

        for (uint8_t col = 0; col < 8; col++) {
            // Check if the current bit is set
            if (line & (0x80 >> col)) {
                // Draw the pixel if the bit is set
                PutPixel(x + col, y + row, color, framebuffer);
            }
        }
    }
}

void RenderString(uint32_t x, uint32_t y, uint32_t color, uint32_t *framebuffer,
                  const char *str, uint32_t screen_width)
{
    uint32_t cursor_x = x;
    uint32_t cursor_y = y;

    while (*str) {
        if (*str == '\n') {
            // Handle newlines
            cursor_x = x;
            cursor_y += 16; // Move down by 16 pixels (or the char height)
        } else {
            // Render the character
            RenderChar(cursor_x, cursor_y, color, framebuffer, *str, screen_width);
            cursor_x += 8; // Move to the next character position

            if (cursor_x + 8 > screen_width) {
                // Wrap to the next line if out of bounds
                cursor_x = x;
                cursor_y += 16;
            }
        }

        str++;
    }
}

void kernel_main()
{
	DbgInit();

	DbgPrint("kernel_main() called\n\r");

    FirstTimeBootInit();
    DbgPrint("VGA Text Mode Initialized\n\r");
    print("Free95 is starting...\n");

	memset(gdt_real, 0x00, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, FREE95_TOTAL_GDT_SEGMENTS);

    gdt_load(gdt_real, sizeof(gdt_real));

    DbgPrint("GDT Initialized\n\r");

	kheap_init();

	DbgPrint("Kernel Heap Initialized\n\r");

	fs_init();

	DbgPrint("Filesystem Initialized\n\r");

    idt_init();

    DbgPrint("IDT Initialized\n\r");

    memset(&tss, 0x00, sizeof(tss));
    tss.esp0 = 0x600000;
    tss.ss0 = KERNEL_DATA_SELECTOR;
    tss_load(0x28);
    
    DbgPrint("TSS Initialized\n\r");

    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    
    paging_switch(paging_4gb_chunk_get_directory(kernel_chunk));

    char *ptr = kzalloc(4096);
    paging_set(paging_4gb_chunk_get_directory(kernel_chunk), (void*)0x1000, (uint32_t)ptr | PAGING_ACCESS_FROM_ALL | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE);

    enable_paging();

	DbgPrint("Enabled Paging\n\rPaging Test:\n\r");

	char *ptr2 = (char*)0x1000;
	ptr2[0] = 'A';
	ptr2[1] = 'B';
	
	DbgPrint(ptr2);
	DbgPrint("\n\r");

	DiskInit();
	
	DbgPrint("Disk Driver Initialized\n\r");

    enable_interrupts();

    DbgPrint("Enabled Interrupts\n\r");

	int fd = fopen("0:/boot.ini", "r");

	if (fd)
	{
		DbgPrint("Boot config file exists\n\r");
		char buf[512];
		fread(buf, 511, 1, fd);
		buf[500] = 0x00;
		DbgPrint("\nRead boot.ini:\n");
		DbgPrint(buf);

		char *settings_section = strstr(buf, "[settings]");
	    if (settings_section)
	    {
	        char *res_start = strstr(settings_section, "res=");
	        if (res_start)
	        {
	            res_start += 4;

	            if (fetchwh(res_start, &w, &h) == 2)
	            {
	                DbgPrint("Resolution was successfully extracted.\n");
	            }
	            else
	            {
	                DbgPrint("Error: Failed to parse resolution.\n");
	            }
	        }
	        else
	        {
	            DbgPrint("Error: \"res=\" was not found in settings section.\n");
	        }
	    }
	    else
	    {
	        DbgPrint("Using default 640x480 resolution.\n");
	    }
	}
	else
	{
		DbgPrint("Error: Boot configuration file not found!\n\r");
		print("boot.ini not found\nFree95 failed to start.\n");
		return;
	}

	terminal_initialize();

	VdiSetScreenRes(w, h);
	VdiInit();
	DbgPrint("\n\rBGA Graphics Driver Initialized\n\r");

	buffer = (uint32_t*)kmalloc(w  * h * 32 / 8);
	fb = (uint32_t*)0xFD000000;

	if (!buffer)
	{
		DbgPrint("\n\rFailed to allocate memory for Double Buffer!\n\r");
		return;
	}

	DbgPrint("Initializing User Mode\n");

    print("CPU Model: ");
    get_cpu_info();
    print("\n\n");

    LoadPsfFont("0:/font.psf");

    jump_usermode();
}
