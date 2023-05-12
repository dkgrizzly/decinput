#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>

#include "mouse.h"
#include "tablet.h"
#include "keyboard.h"

bool tablet_enabled = false;

void core1_loop() {
    for(;;) {
        keyboard_dowork();
    }
}

int main() {
    keyboard_init();

    if(tablet_enabled) {
        tablet_init();
    } else {
        mouse_init();
    }

    multicore_launch_core1(core1_loop);

    for(;;) {
        if(tablet_enabled) {
            tablet_dowork();
        } else {
            mouse_dowork();
        }
    }
}
