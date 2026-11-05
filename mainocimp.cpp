#include "pico/stdlib.h"
#include "overcurrent.h"

int main() {
    stdio_init_all();
    oc::init();                     

    //insert current drive main probably

    bool was_allowed = true;
    while (true) {
        bool allowed = oc::motors_allowed();
        if (!allowed) {
            if (was_allowed) printf("OC trip %lu\n", (unsigned long)oc::trip_count());
            stop_all_motors();    //placeholder
        }
        else {
            if (!was_allowed) ramp_up_motors();   //same here
        }
        was_allowed = allowed;
    }
}