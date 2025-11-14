#include <rtc.h>
#include <unistd.h>

int main(void) {
    if (rtc_init() != 0) return 1;
    rtc_print_time();
    return 0;
}
