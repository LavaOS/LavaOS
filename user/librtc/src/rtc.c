/* rtc.c - minimal user-space RTC + TSC sync for limited libc environments
   Requires permission to use I/O ports (root or CAP_SYS_RAWIO). */

#include "rtc.h"
#include <stdint.h>
/* printf is optional; if libc really lacks it, remove rtc_print_time or replace */
#include <stdio.h>

/* ---------------- low-level I/O (inline asm) ----------------
   Note: accessing ports requires privileges (root / CAP_SYS_RAWIO).
*/
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* ---------------- CMOS read (must be visible before use) ---------- */
static inline uint8_t cmos_read(uint8_t reg)
{
    /* Do not touch NMI bit (bit7) unless you know what you're doing.
       If you need to disable NMI, OR reg with 0x80. */
    outb(0x70, reg);
    return inb(0x71);
}

/* ---------------- TSC helpers ---------------- */
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/* Shared state */
static uint64_t tsc_hz = 0;
static rtc_time_t rtc_base_time = {0,0,0};
static uint64_t rtc_base_tsc = 0;

/* BCD -> binary */
static inline uint8_t bcd_to_bin(uint8_t v) {
    return (v & 0x0F) + ((v >> 4) * 10);
}

/* Wait until RTC update-in-progress flag clears */
static void rtc_wait_update_clear(void) {
    /* bit 7 of reg 0x0A is Update-In-Progress (UIP) */
    while (cmos_read(0x0A) & 0x80) {
        __asm__ volatile ("pause");
    }
}

/* Read raw RTC time (seconds/minutes/hours), convert BCD and 12h -> 24h if needed */
static rtc_time_t rtc_read_raw(void) {
    rtc_wait_update_clear();

    uint8_t sec  = cmos_read(0x00);
    uint8_t min  = cmos_read(0x02);
    uint8_t hour = cmos_read(0x04);
    uint8_t regB = cmos_read(0x0B);

    if (!(regB & 0x04)) { /* data in BCD */
        sec  = bcd_to_bin(sec);
        min  = bcd_to_bin(min);
        hour = bcd_to_bin(hour);
    }

    if (!(regB & 0x02)) { /* 12-hour mode */
        if (hour & 0x80) hour = (hour & 0x7F) + 12;
    }

    rtc_time_t t = { hour, min, sec };
    return t;
}

/* ---------------- calibrate_tsc via two RTC-second edges ---------------
   Wait for a second tick change, record t0, wait next second tick change, record t1.
   diff = t1 - t0 equals exactly 1 second in TSC ticks -> tsc_hz = diff.
   This blocks ~1 second (RTC-driven) but is the most accurate and requires
   no libc timing functions.
*/
static int calibrate_tsc_via_rtc(void) {
    /* Check we can read CMOS once (permission). If read returns 0xFF repeatedly it may be inaccessible. */
    uint8_t a = cmos_read(0x00);
    (void)a; /* just probe */

    /* wait until second changes (edge 1) */
    uint8_t sec = cmos_read(0x00);
    while (cmos_read(0x00) == sec) {
        __asm__ volatile ("pause");
    }

    uint64_t t0 = rdtsc();

    /* wait for next second edge (edge 2) */
    sec = cmos_read(0x00);
    while (cmos_read(0x00) == sec) {
        __asm__ volatile ("pause");
    }

    uint64_t t1 = rdtsc();

    uint64_t diff = t1 - t0;
    if (diff == 0) return -1;

    tsc_hz = diff; /* one-second worth of TSC ticks */
    return 0;
}

/* Public API implementation */

int rtc_init(void) {
    /* calibrate TSC using RTC edges */
    if (calibrate_tsc_via_rtc() != 0) {
        /* calibration failed (maybe no port access) */
        return -1;
    }

    /* snapshot RTC base */
    rtc_base_time = rtc_read_raw();
    rtc_base_tsc = rdtsc();
    return 0;
}

rtc_time_t rtc_now(void) {
    /* If tsc_hz==0 fallback to raw CMOS read */
    if (tsc_hz == 0) {
        return rtc_read_raw();
    }

    rtc_time_t res = rtc_base_time;
    uint64_t dt_ticks = rdtsc() - rtc_base_tsc;
    uint64_t dt_sec = dt_ticks / tsc_hz;

    uint64_t total_seconds = (uint64_t)res.second + dt_sec;
    res.second = total_seconds % 60;
    uint64_t carry = total_seconds / 60;

    uint64_t total_minutes = (uint64_t)res.minute + carry;
    res.minute = total_minutes % 60;
    carry = total_minutes / 60;

    uint64_t total_hours = (uint64_t)res.hour + carry;
    res.hour = total_hours % 24;

    return res;
}

void rtc_print_time(void) {
    rtc_time_t t = rtc_now();
    /* printf may not exist in very tiny libc; if so, replace with your kernel/user printing */
    printf("UTC Time is: %02u:%02u:%02u\n", t.hour, t.minute, t.second);
}
