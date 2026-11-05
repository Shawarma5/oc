#include "overcurrent.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

namespace oc {
    namespace {

        constexpr uint kResetPin = 10;     
        constexpr uint kStatusPin = 11;     

        constexpr uint32_t kHoldMs = 1500; // motor off time after trip
        constexpr uint32_t kPulseMs = 200;  // reset

        volatile bool     fault = false;
        volatile uint32_t trips = 0;

        void start_fault();

        // fault check
        int64_t end_reset(alarm_id_t, void*) {
            gpio_put(kResetPin, 0);
            if (gpio_get(kStatusPin)) {
                fault = false;              // clear
            }
            else {
                start_fault();              // not
            }
            return 0;                       //
        }

        int64_t begin_reset(alarm_id_t, void*) {
            gpio_put(kResetPin, 1);
            add_alarm_in_ms(kPulseMs, end_reset, nullptr, true);
            return 0;
        }

        // trip
        void start_fault() {
            fault = true;
            trips = trips + 1;
            add_alarm_in_ms(kHoldMs, begin_reset, nullptr, true);
        }

        // latch
        void status_irq() {
            if (gpio_get_irq_event_mask(kStatusPin) & GPIO_IRQ_EDGE_FALL) {
                gpio_acknowledge_irq(kStatusPin, GPIO_IRQ_EDGE_FALL);
                if (!fault) start_fault();
            }
        }

    } 

    void init() {
        gpio_init(kResetPin);
        gpio_set_dir(kResetPin, GPIO_OUT);
        gpio_put(kResetPin, 0);

        gpio_init(kStatusPin);
        gpio_set_dir(kStatusPin, GPIO_IN);
        gpio_disable_pulls(kStatusPin);                         

        
        gpio_put(kResetPin, 1);
        sleep_ms(kPulseMs);
        gpio_put(kResetPin, 0);
        sleep_ms(5);

        gpio_acknowledge_irq(kStatusPin, GPIO_IRQ_EDGE_FALL);   
        if (!gpio_get(kStatusPin)) start_fault();               

        gpio_add_raw_irq_handler(kStatusPin, status_irq);
        gpio_set_irq_enabled(kStatusPin, GPIO_IRQ_EDGE_FALL, true);
        irq_set_enabled(IO_IRQ_BANK0, true);
    }

    bool motors_allowed() { return !fault; }
    uint32_t trip_count() { return trips; }

}  