#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include "printk.h"

/* TSC from your delay system */
extern uint64_t tsc_hz;

/* ------------------------- RTC structure -------------------------- */
typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} rtc_time_t;

/* last time snapshot */
static rtc_time_t rtc_base_time;
static uint64_t rtc_base_tsc = 0;

/* read 1 CMOS register */
static uint8_t cmos_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

/* wait until RTC update not in progress */
static void rtc_wait_ready() {
    while (cmos_read(0x0A) & 0x80) { }
}

/* convert BCD → binary if needed */
static uint8_t bcd_to_binary(uint8_t x) {
    return (x & 0x0F) + ((x / 16) * 10);
}

/* ------------------------- Read RTC (UTC) -------------------------- */
static rtc_time_t rtc_read_raw() {
    rtc_wait_ready();

    uint8_t sec  = cmos_read(0x00);
    uint8_t min  = cmos_read(0x02);
    uint8_t hour = cmos_read(0x04);

    uint8_t regB = cmos_read(0x0B);

    if (!(regB & 0x04)) {      // If BCD mode
        sec  = bcd_to_binary(sec);
        min  = bcd_to_binary(min);
        hour = bcd_to_binary(hour);
    }

    if (!(regB & 0x02)) {      // 12-hour mode
        if (hour & 0x80) hour = (hour & 0x7F) + 12;
    }

    rtc_time_t t = { hour, min, sec };
    return t;
}

/* ------------------------- INIT RTC -------------------------- */
static void rtc_init() {
    rtc_base_time = rtc_read_raw();
    rtc_base_tsc = rdtsc();

    printk("RTC initialized at UTC %02u:%02u:%02u\n",
           rtc_base_time.hour, rtc_base_time.minute, rtc_base_time.second);
}

/* ------------------------- GET CURRENT TIME -------------------------- */
static rtc_time_t rtc_now() {
    /* compute seconds passed using TSC */
    uint64_t dt_ticks = rdtsc() - rtc_base_tsc;
    uint64_t dt_sec   = dt_ticks / tsc_hz;

    rtc_time_t t = rtc_base_time;

    t.second += dt_sec;

    /* Normalize */
    if (t.second >= 60) {
        t.minute += t.second / 60;
        t.second %= 60;
    }
    if (t.minute >= 60) {
        t.hour += t.minute / 60;
        t.minute %= 60;
    }
    if (t.hour >= 24) {
        t.hour %= 24;
    }

    return t;
}

/* ------------------------ PRINT UTC TIME ------------------------- */
static void rtc_print_time() {
    rtc_time_t t = rtc_now();
    printk("UTC Time is: %02u:%02u:%02u\n", t.hour, t.minute, t.second);
}

#endif // RTC_H
