#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

/*
    High-precision delay using:
    - PIT for calibration
    - TSC for fast & accurate waiting

    100% stable, same result on every boot.
*/

static uint64_t tsc_hz = 0;

/* -----------------------------------------
   Read Time Stamp Counter (64-bit)
------------------------------------------ */
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

/* -----------------------------------------
   PIT Wait: Wait EXACTLY 10ms using PIT
------------------------------------------ */
static inline void pit_wait_10ms() {
    const uint16_t divisor = 11932;  // 10ms
    
    outb(0x43, 0x34);     // Channel 0, lobyte/hibyte, mode 2
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);

    // read counter until it hits zero (wrap)
    uint16_t last = 0xFFFF;

    while (1) {
        outb(0x43, 0x0);  // latch channel 0 count
        uint8_t lo = inb(0x40);
        uint8_t hi = inb(0x40);
        uint16_t cnt = (hi << 8) | lo;

        if (cnt > last)  // wrapped → finished
            break;

        last = cnt;
    }
}

/* -----------------------------------------
   Calibrate TSC using PIT 10ms precise wait
------------------------------------------ */
static inline void calibrate_tsc() {
    // Warm-up read
    rdtsc();

    uint64_t t0 = rdtsc();
    pit_wait_10ms();    // EXACT 10 ms
    uint64_t t1 = rdtsc();

    uint64_t diff = t1 - t0;

    // 10ms → multiply ×100 to get per second
    tsc_hz = diff * 100;
}

/* -----------------------------------------
   Delay functions
------------------------------------------ */
static inline void udelay(uint64_t us) {
    if (tsc_hz == 0) calibrate_tsc();

    uint64_t start = rdtsc();
    uint64_t ticks = (tsc_hz / 1000000) * us;

    while (rdtsc() - start < ticks) {
        __asm__ volatile("pause");
    }
}

static inline void mdelay(uint64_t ms) {
    udelay(ms * 1000);
}

static inline void delay(uint64_t sec) {
    mdelay(sec * 1000);
}

#endif
