#include <stdint.h>
#include <stdio.h>

#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/time.h>

#include "screen.h"
#include "util.h"

void run()
{
    sleep_ms(2000);
    WARN(SW_SCR_BEGIN);
    sw_scr_run();
    WARN(SW_SCR_END);
    while (true) {
        printf("CORE 1 running with %s\n", __func__);
        sleep_ms(1000);
    }
}

bool repeating_alarm_cb(struct repeating_timer* timer)
{
    printf("%d INTERRUPT_BEGIN %s:%3d ->[%s]\n", timer->alarm_id, __func__,
           __LINE__, (char*)timer->user_data);
    fflush(stdout);
    absolute_time_t t = get_absolute_time();
    uint64_t c = 0;
    do {
        c++;
    } while (absolute_time_diff_us(t, get_absolute_time()) < 10000000);

    printf("%ld - %d INTERRUPT_END  %s:%3d ->[%s]\n",

           absolute_time_diff_us(t, get_absolute_time()), timer->alarm_id,
           __func__, __LINE__, (char*)timer->user_data);
    return c > 0;
}

int64_t alarm_cb(int32_t r, void* data)
{
    printf("%d INTERRUPT_BEGIN %s:%3d ->[%s]\n", r, __func__, __LINE__,
           (char*)data);
    sleep_ms(2000);
    printf("\r%d INTERRUPT_END  %s:%3d ->[%s]\n", r, __func__, __LINE__,
           (char*)data);
    add_alarm_in_ms(-10000, alarm_cb, "Hello from callback later", false);
    return 0;
}

int main(int argc, char* argv[])
{
    stdio_init_all();
    sw_scr_init();
    multicore_launch_core1(run);

    while (1) {
        //        printf("CORE 0 running with %s\n", __func__);
        sleep_ms(1000);
    }

    return 0;
}
