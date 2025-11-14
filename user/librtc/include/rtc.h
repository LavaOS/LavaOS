#include <stdint.h>

typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} rtc_time_t;

/* Public API */
int rtc_init(void);          /* returns 0 on success */
rtc_time_t rtc_now(void);
void rtc_print_time(void);
