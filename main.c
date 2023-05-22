#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/gpio.h>

#include "bsp/board.h"
#include "tusb.h"

#include "mouse.h"
#include "tablet.h"
#include "keyboard.h"

void led_blinking_task(void);
extern void cdc_app_task(void);
extern void hid_app_task(void);

bool tablet_enabled = false;

void core1_loop() {
    for(;;) {
        keyboard_dowork();
        if(tablet_enabled) {
            tablet_dowork();
        } else {
            mouse_dowork();
        }
    }
}

int main() {
    keyboard_init();

    if(tablet_enabled) {
        tablet_init();
    } else {
        mouse_init();
    }

    board_init();

    // Red LED
    gpio_init(11);
    gpio_set_dir(11, GPIO_OUT);
    gpio_put(11, 0);
    
    // Turn off the NeoPixel
    gpio_init(16);
    gpio_set_dir(16, GPIO_OUT);
    gpio_put(16, 0);
    gpio_set_dir(17, GPIO_OUT);
    gpio_put(17, 0);
    
    tuh_init(BOARD_TUH_RHPORT);

    multicore_launch_core1(core1_loop);

    for(;;) {
        tuh_task();
        cdc_app_task();
        hid_app_task();
        led_blinking_task();
    }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

void tuh_mount_cb(uint8_t dev_addr)
{
  // application set-up
  //printf("A device with address %d is mounted\r\n", dev_addr);
}

void tuh_umount_cb(uint8_t dev_addr)
{
  // application tear-down
  //printf("A device with address %d is unmounted \r\n", dev_addr);
}


//--------------------------------------------------------------------+
// Blinking Task
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  const uint32_t interval_ms = 1000;
  static uint32_t start_ms = 0;

  static bool led_state = false;

  // Blink every interval ms
  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  gpio_put(11, led_state);
  led_state = 1 - led_state; // toggle
}
