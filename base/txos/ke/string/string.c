/*++

Free95 20x/TX Kernel

You may only use this code if you agree to the terms of the Free95 Source Code License agreement (GNU GPL v3) (see LICENSE).
If you do not agree to the terms, do not use the code.


Module Name:

    string.c

Abstract:

    This module implements basic string operation services.

--*/

#include "string.h"

char tolower(char s1)
{
    if (s1 >= 65 && s1 <= 90)
    {
        s1 += 32;
    }

    return s1;
}

int strlen(const char* ptr)
{
    int i = 0;
    while(*ptr != 0)
    {
        i++;
        ptr += 1;
    }

    return i;
}

int strnlen(const char* ptr, int max)
{
    int i = 0;
    for (i = 0; i < max; i++)
    {
        if (ptr[i] == 0)
            break;
    }

    return i;
}

int strnlen_terminator(const char* str, int max, char terminator)
{
    int i = 0;
    for(i = 0; i < max; i++)
    {
        if (str[i] == '\0' || str[i] == terminator)
            break;
    }

    return i;
}

int istrncmp(const char* s1, const char* s2, int n)
{
    unsigned char u1, u2;
    while(n-- > 0)
    {
        u1 = (unsigned char)*s1++;
        u2 = (unsigned char)*s2++;
        if (u1 != u2 && tolower(u1) != tolower(u2))
            return u1 - u2;
        if (u1 == '\0')
            return 0;
    }

    return 0;
}

int strncmp(const char* str1, const char* str2, int n)
{
    unsigned char u1, u2;

    while(n-- > 0)
    {
        u1 = (unsigned char)*str1++;
        u2 = (unsigned char)*str2++;
        if (u1 != u2)
            return u1 - u2;
        if (u1 == '\0')
            return 0;
    }

    return 0;
}

int strcmp(const char *str1, const char *str2)
{
    while (*str1 && *str2 && *str1 == *str2)
    {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

char* strcpy(char* dest, const char* src)
{
    char* res = dest;
    while(*src != 0)
    {
        *dest = *src;
        src += 1;
        dest += 1;
    }

    *dest = 0x00;

    return res;
}

char* strncpy(char* dest, const char* src, int count)
{
    int i = 0;
    for (i = 0; i < count-1; i++)
    {
        if (src[i] == 0x00)
            break;

        dest[i] = src[i];
    }

    dest[i] = 0x00;
    return dest;
}

bool isdigit(char c)
{
    return c >= 48 && c <= 57;
}

int tonumericdigit(char c)
{
    return c - 48;
}

char *strtok(char *str, const char *delim)
{
    static char *nextToken = NULL; // Static pointer to maintain state across calls
    if (str != NULL) {
        nextToken = str; // Initialize nextToken if a new string is provided
    }
    if (nextToken == NULL) {
        return NULL; // No more tokens
    }

    // Skip leading delimiters
    while (*nextToken && strchr(delim, *nextToken)) {
        nextToken++;
    }

    if (*nextToken == '\0') {
        return NULL; // No token left
    }

    // Mark the start of the token
    char *start = nextToken;

    // Find the end of the token
    while (*nextToken && !strchr(delim, *nextToken)) {
        nextToken++;
    }

    if (*nextToken) {
        *nextToken = '\0'; // Null-terminate the token
        nextToken++;       // Advance to the next character
    } else {
        nextToken = NULL; // End of the string
    }

    return start;
}

char *strchr(const char *str, int ch)
{
    // Iterate through the string
    while (*str) {
        if (*str == (char)ch) {
            return (char *)str; // Return pointer to the first occurrence of 'ch'
        }
        str++;
    }

    // Check for null character match
    if (ch == '\0') {
        return (char *)str; // Return pointer to the null character
    }

    return NULL; // Character not found
}
