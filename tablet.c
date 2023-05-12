#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>

typedef struct tablet_status_s {
    bool selftest_done;

    uint8_t buttons_held;
    uint8_t buttons_pressed;
    uint8_t buttons_released;

    uint8_t laststate;

    uint16_t x, lx;
    uint16_t y, ly;
    
    uint16_t baud;
    uint8_t mode;

    struct repeating_timer timer;
} tablet_status_t;

tablet_status_t tablet_status;

static void tablet_report() {
    uint8_t ts = 0x40;

    // Handle button debouncing, make sure the host sees any pressing before the release is processed.
    tablet_status.buttons_held |= tablet_status.buttons_pressed;
    tablet_status.buttons_pressed = 0;

    ts |= (tablet_status.buttons_held & 0xf) << 1;

    tablet_status.buttons_held &= ~tablet_status.buttons_released;
    tablet_status.buttons_released = 0;

    // In polled mode, always report.
    // In stream mode, only report if we have significant changes.
    if(tablet_status.mode == 'P')
        ts |= 0x80;
    else if((tablet_status.lx & 0xFFE) != (tablet_status.x & 0xFFE))
        ts |= 0x80;
    else if((tablet_status.ly & 0xFFE) != (tablet_status.y & 0xFFE))
        ts |= 0x80;
    else if((tablet_status.laststate & 0x1F) != (ts & 0x1F))
        ts |= 0x80;
    
    tablet_status.laststate = ts;

    if(ts & 0x80) {
        // DEC Serial Tablet Packet
        uart_putc_raw(uart1, ts);
        uart_putc_raw(uart1, tablet_status.x & 0x3f);
        uart_putc_raw(uart1, (tablet_status.x >> 6) & 0x3f);
        uart_putc_raw(uart1, tablet_status.y & 0x3f);
        uart_putc_raw(uart1, (tablet_status.y >> 6) & 0x3f);
    }
}

static bool tablet_stream_callback(struct repeating_timer *t) {
    if(tablet_status.selftest_done && (tablet_status.mode == 'R'))
        tablet_report();

    return true;
}

static void tablet_set_reportrate(uint rate) {
    cancel_repeating_timer(&tablet_status.timer);

    if(rate == 120) {
        add_repeating_timer_ms(-9, tablet_stream_callback, NULL, &tablet_status.timer);
    } else if(rate == 72) {
        add_repeating_timer_ms(-14, tablet_stream_callback, NULL, &tablet_status.timer);
    } else {
        add_repeating_timer_ms(-18, tablet_stream_callback, NULL, &tablet_status.timer);
    }
}

static void tablet_selftest() {
    uart_tx_wait_blocking(uart1);
    uart_set_baudrate(uart1, 4800);

    uart_putc_raw(uart1, 0xA0);  // Self Test Report, REV0
    uart_putc_raw(uart1, 0x04);  // Manufacturing ID 0, Tablet
    uart_putc_raw(uart1, 0x00);  // No Errors
    uart_putc_raw(uart1, 0x00);  // No Buttons Held

    tablet_status.baud = 4800;
    tablet_status.mode = 'D';
    tablet_status.lx = 0;
    tablet_status.ly = 0;
    tablet_status.x = 0;
    tablet_status.y = 0;
    tablet_status.laststate = 0;
    tablet_status.buttons_released = 0;
    tablet_status.buttons_pressed = 0;
    tablet_status.buttons_held = 0;

    tablet_set_reportrate(55);

    // Drain FIFO
    while(uart_is_readable(uart1)) {
        uart_getc(uart1);
    }

    tablet_status.selftest_done = true;
}

static int64_t tablet_selftest_callback(alarm_id_t id, void *user_data) {
    tablet_selftest();
    return 0;
}

void tablet_dowork() {
    if(uart_get_hw(uart1)->rsr & UART_UARTRSR_BE_BITS) {
        // Serial BREAK condition
        hw_clear_bits(&uart_get_hw(uart1)->rsr, UART_UARTRSR_BITS);

        // Execute self test
        tablet_status.mode = 'T';
    } else if(uart_is_readable(uart1)) {
        uint8_t incomingByte = uart_getc(uart1);
        if(tablet_status.selftest_done) {
            switch(incomingByte) {
            case 'B':
                // Change baud rate to 9600
                tablet_status.baud = 9600;
                uart_tx_wait_blocking(uart1);
                uart_set_baudrate(uart1, 9600);
                break;
            case 'S':
                // Stream Report Format
                break;
            case 'R':
                // Stream Mode
                tablet_status.mode = 'R';
                break;
            case 'K':
                // Change report rate to ~55/s
                tablet_set_reportrate(55);
                break;
            case 'L':
                // Change report rate to ~72/s
                tablet_set_reportrate(72);
                break;
            case 'M':
                // Change report rate to ~120/s
                if(tablet_status.baud == 9600) {
                    tablet_set_reportrate(120);
                }
                break;
            case 'P':
                // Poll Read
                tablet_status.mode = 'P';
                break;
            case 'D':
                // Poll Mode
                tablet_status.mode = 'D';
                break;
            case 'T':
                // Self Test
                tablet_status.mode = 'T';
                break;
            default:
                // Unknown Byte
                break;
            }
        }
    }
    
    if(!tablet_status.selftest_done) {
        return;
    }

    switch(tablet_status.mode) {
    case 'P':
        tablet_report();
        tablet_status.mode = 'D';
        break;
    case 'T':
        tablet_selftest();
        break;
    }
}

void tablet_init() {
    uart_init(uart1, 4800);
    uart_set_format(uart1, 8, 1, UART_PARITY_ODD);
    gpio_set_function(8, GPIO_FUNC_UART);
    gpio_set_function(9, GPIO_FUNC_UART);
    
    memset(&tablet_status, 0, sizeof(tablet_status));
    tablet_status.baud = 4800;
    tablet_status.mode = 'D';

    add_alarm_in_ms(1000, tablet_selftest_callback, NULL, false);

    add_repeating_timer_ms(-18, tablet_stream_callback, NULL, &tablet_status.timer);
}