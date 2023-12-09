#include <stdint.h>
#include <stdio.h>

#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/time.h>

#include "sw_apps/apps.h"
#include "sw_os/dev.h"
#include "sw_os/state.h"
#include "sw_utils/util.h"

void run() { apps_load_clock(); }

int main(int argc, char* argv[])
{
    stdio_init_all();
    os_init();
    apps_init();
    multicore_launch_core1(run);

    while (1) {
        //        printf("CORE 0 running with %s\n", __func__);
        sleep_ms(1000);
        GyroData d = os_gyro_fetch();
        printf(GYRO_S(d));
    }

    return 0;
}
