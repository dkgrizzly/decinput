#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>

typedef struct mouse_status_s {
    bool selftest_done;

    uint8_t buttons_held;
    uint8_t buttons_pressed;
    uint8_t buttons_released;

    uint8_t laststate;

    int16_t dx;
    int16_t dy;
    
    uint16_t baud;
    uint8_t mode;

    struct repeating_timer timer;
} mouse_status_t;

mouse_status_t mouse_status;

static void mouse_report() {
    uint8_t mx, my;
    uint8_t ms = 0x00;

    // Translate X movement
    if(mouse_status.dx == 0) {
        mx = 0;
    } else if(mouse_status.dx < -127) {
        // Send -127 to host, adjust local cursor difference.
        mouse_status.dx += 127;
        mx = 0x7f;
        ms |= 0x10;
    } else if(mouse_status.dx > 127) {
        // Send +127 to host, adjust local cursor difference.
        mouse_status.dx -= 127;
        mx = 0x7f;
    } else if(mouse_status.dx < 0) {
        mx = 0 - mouse_status.dx;
        ms |= 0x10;
        mouse_status.dx = 0;
    } else {
        mx = mouse_status.dx;
        mouse_status.dx = 0;
    }

    // Translate Y movement
    if(mouse_status.dy == 0) {
        my = 0;
    } else if(mouse_status.dy < -127) {
        // Send -127 to host, adjust local cursor difference.
        mouse_status.dy += 127;
        my = 0x7f;
        ms |= 0x08;
    } else if(mouse_status.dy > 127) {
        // Send +127 to host, adjust local cursor difference.
        mouse_status.dy -= 127;
        my = 0x7f;
    } else if(mouse_status.dy < 0) {
        my = 0 - mouse_status.dy;
        ms |= 0x08;
        mouse_status.dy = 0;
    } else {
        my = mouse_status.dy;
        mouse_status.dy = 0;
    }
    
    // Handle button debouncing, make sure the host sees any pressing before the release is processed.
    mouse_status.buttons_held |= mouse_status.buttons_pressed;
    mouse_status.buttons_pressed = 0;

    ms |= (mouse_status.buttons_held & 0x7);

    mouse_status.buttons_held &= ~mouse_status.buttons_released;
    mouse_status.buttons_released = 0;

    // In polled mode, always report.
    // In stream mode, only report if we have significant changes.
    if(mouse_status.mode == 'P')
        ms |= 0x80;
    else if(mx & 0x7E)
        ms |= 0x80;
    else if(my & 0x7E)
        ms |= 0x80;
    else if((mouse_status.laststate & 0x7) != (ms & 0x7))
        ms |= 0x80;

    mouse_status.laststate = ms;

    // DEC Serial Mouse Packet
    if(ms & 0x80) {
        uart_putc_raw(uart1, ms);
        uart_putc_raw(uart1, mx & 0x7f);
        uart_putc_raw(uart1, my & 0x7f);
    }
}

static bool mouse_stream_callback(struct repeating_timer *t) {
    if(mouse_status.selftest_done && (mouse_status.mode == 'R'))
        mouse_report();

    return true;
}

static void mouse_set_reportrate(uint rate) {
    cancel_repeating_timer(&mouse_status.timer);

    if(rate == 120) {
        add_repeating_timer_ms(-9, mouse_stream_callback, NULL, &mouse_status.timer);
    } else if(rate == 72) {
        add_repeating_timer_ms(-14, mouse_stream_callback, NULL, &mouse_status.timer);
    } else {
        add_repeating_timer_ms(-18, mouse_stream_callback, NULL, &mouse_status.timer);
    }
}

static void mouse_selftest() {
    uart_tx_wait_blocking(uart1);
    uart_set_baudrate(uart1, 4800);

    uart_putc_raw(uart1, 0xA0);  // Self Test Report, REV0
    uart_putc_raw(uart1, 0x02);  // Manufacturing ID 0, Mouse
    uart_putc_raw(uart1, 0x00);  // No Errors
    uart_putc_raw(uart1, 0x00);  // No Buttons Held

    mouse_status.baud = 4800;
    mouse_status.mode = 'D';
    mouse_status.dx = 0;
    mouse_status.dy = 0;
    mouse_status.laststate = 0;
    mouse_status.buttons_released = 0;
    mouse_status.buttons_pressed = 0;
    mouse_status.buttons_held = 0;

    // Drain FIFO
    while(uart_is_readable(uart1)) {
        uart_getc(uart1);
    }

    mouse_status.selftest_done = true;
}

static int64_t mouse_selftest_callback(alarm_id_t id, void *user_data) {
    mouse_selftest();
    return 0;
}

void mouse_dowork() {
    if(uart_get_hw(uart1)->rsr & UART_UARTRSR_BE_BITS) {
        // Serial BREAK condition
        hw_clear_bits(&uart_get_hw(uart1)->rsr, UART_UARTRSR_BITS);
        // Execute self test
        mouse_status.mode = 'T';
    } else if(uart_is_readable(uart1)) {
        uint8_t incomingByte = uart_getc(uart1);
        if(mouse_status.selftest_done) {
            switch(incomingByte) {
            case 'B':
                // Change baud rate to 9600
                mouse_status.baud = 9600;
                uart_tx_wait_blocking(uart1);
                uart_set_baudrate(uart1, 9600);
                break;
            case 'S':
                // Stream Report Format
                break;
            case 'R':
                // Stream Mode
                mouse_status.mode = 'R';
                break;
            case 'K':
                // Change report rate to ~55/s
                mouse_set_reportrate(55);
                break;
            case 'L':
                // Change report rate to ~72/s
                mouse_set_reportrate(72);
                break;
            case 'M':
                // Change report rate to ~120/s
                if(mouse_status.baud == 9600) {
                    mouse_set_reportrate(120);
                }
                break;
            case 'P':
                // Poll Read
                mouse_status.mode = 'P';
                break;
            case 'D':
                // Poll Mode
                mouse_status.mode = 'D';
                break;
            case 'T':
                // Self Test
                mouse_status.mode = 'T';
                break;
            default:
                // Unknown Byte
                break;
            }
        }
    }
    
    if(!mouse_status.selftest_done) {
        return;
    }

    switch(mouse_status.mode) {
    case 'P':
        mouse_report();
        mouse_status.mode = 'D';
        break;
    case 'T':
        mouse_selftest();
        break;
    }
}

void mouse_init() {
    uart_init(uart1, 4800);
    uart_set_format(uart1, 8, 1, UART_PARITY_ODD);
    gpio_set_function(8, GPIO_FUNC_UART);
    gpio_set_function(9, GPIO_FUNC_UART);
    
    memset(&mouse_status, 0, sizeof(mouse_status));
    mouse_status.baud = 4800;
    mouse_status.mode = 'D';

    add_alarm_in_ms(1000, mouse_selftest_callback, NULL, false);

    add_repeating_timer_ms(-18, mouse_stream_callback, NULL, &mouse_status.timer);
}