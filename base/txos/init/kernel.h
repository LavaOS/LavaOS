#ifndef KERNEL_H
#define KERNEL_H

#include <stddef.h>
#include <stdint.h>

#define PORT 0x3f8          // COM1 serial port
#define VGA_WIDTH 80
#define VGA_HEIGHT 20

#define FREE95_MAX_PATH 108

#define LOG_SUCCESS 1
#define LOG_FAIL 0
#define LOG_ERROR 2
#define LOG_WARN 3
#define LOG_INFO 4

void kernel_main();
void terminal_initialize();
void terminal_writechar(char c, char colour);
void snprintf(char *buffer, size_t size, const char *format, ...);
void print(const char* str);
void DbgPutc(char a);
void DbgPrint(const char *format, ...);
void DbgLog(const char *msg, int type);
void SetExecBuffer(char *b);
void PrintChar(char str);
void Print(const char *str);
void ClearScreen();
void NsExec(char *ex_buffer, int isBat);

#define ERROR(value) (void*)(value)
#define ERROR_I(value) (int)(value)
#define ISERR(value) ((int)value < 0)

#endif
