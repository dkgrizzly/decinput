#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/pwm.h>
#include "bsp/board.h"

#define KBD_ALL_UPS     (0xB3)
#define KBD_METRONOME   (0xB4)
#define KBD_OUTPUT_ERR  (0xB5)
#define KBD_INPUT_ERR   (0xB6)
#define KBD_LOCK_ACK    (0xB7)
#define KBD_TEST_MODE   (0xB8)
#define KBD_PREFIX      (0xB9)
#define KBD_CHMODE_ACK  (0xBA)
#define KBD_RESERVED    (0x7F)

#define KBD_FWID     (0x01)
#define KBD_HWID     (0x00)
#define KBD_KDPO_ERR (0x3D)
#define KBD_POST_ERR (0x3E)

#define FD_MAIN		(1)
#define	FD_NUMPAD	(2)
#define	FD_DELETE	(3)
#define	FD_RETURN	(4)
#define	FD_LOCK		(5)
#define	FD_SHIFT	(6)
#define	FD_HCURSOR	(7)
#define	FD_VCURSOR	(8)
#define	FD_EDITING	(9)
#define	FD_FA		(10)
#define	FD_FB		(11)
#define	FD_FC		(12)
#define	FD_FD		(13)
#define	FD_FE		(14)

#define FDM_DOWN	(0)
#define FDM_AUTO	(1)
#define FDM_DNUP	(3)

typedef struct divstate_s {
    uint8_t mode;
    uint8_t arbuf;
    bool click;
} divstate_t;

typedef struct arbuf_s {
    uint16_t timeout;
    uint16_t interval;
} arbuf_t;

divstate_t divstate[16];
arbuf_t arbuf[4];
bool keystate[256];
bool keys[256];
bool ledstate[4];
uint8_t autoinhibit;
bool inhibit;
bool ctrlclick;
uint8_t keyclick_volume;
uint8_t bell_volume;
uint8_t cmd_param[4];
uint8_t cmd_pos;
uint audio_slice;
uint audio_chan;

static int64_t keyboard_silence_callback(alarm_id_t id, void *user_data) {
    pwm_set_chan_level(audio_slice, audio_chan, 0);
    return 0;
}

void keyboard_sound(uint ms) {
    uint vol;
    
    if(ms == 0) {
        pwm_set_chan_level(audio_slice, audio_chan, 0);
        return;
    } else if(ms == 2) {
        vol = keyclick_volume * 36;
    } else {
        vol = bell_volume * 36;
    }

    pwm_set_chan_level(audio_slice, audio_chan, vol);
    add_alarm_in_ms(ms, keyboard_silence_callback, NULL, false);
}

static void keyboard_defaults() {
    int i;

    keyclick_volume = 2;
    bell_volume = 2;
    ctrlclick = false;
    autoinhibit = 0;
    inhibit = false;

    divstate[1].mode = FDM_AUTO;  // Graphic Keys, Spacebar (ASCII 0x20 - 0x7E)
    divstate[1].click = true;

    divstate[2].mode = FDM_AUTO;  // Numeric Keypad
    divstate[2].click = true;

    divstate[3].mode = FDM_AUTO;  // Delete (E12)
    divstate[3].click = true;
    divstate[3].arbuf = 1;

    divstate[4].mode = FDM_DNUP;  // Return (C13) and Tab (D00)
    divstate[4].click = true;

    divstate[5].mode = FDM_DNUP;  // Lock (C00) and Compose (A99)
    divstate[5].click = true;

    divstate[6].mode = FDM_DNUP;  // Shift (B99 and B11) and Control (C99)

    divstate[7].mode = FDM_AUTO;  // Horizontal Cursors (B16 and B18)
    divstate[7].click = true;
    divstate[7].arbuf = 1;

    divstate[8].mode = FDM_AUTO;  // Vertical Cursors (B17 and C17)
    divstate[8].click = true;
    divstate[8].arbuf = 1;

    divstate[9].mode = FDM_DNUP;  // Ins, Home, PgUp (E16-E18), Del, End, PgDn (D16-D18)
    divstate[9].click = true;

    divstate[10].mode = FDM_AUTO; // Function Keys (G99-G03)
    divstate[10].click = true;

    divstate[11].mode = FDM_AUTO; // Function Keys (G05-G09) 
    divstate[11].click = true;

    divstate[12].mode = FDM_AUTO; // Function Keys (G11-G14)
    divstate[12].click = true;

    divstate[13].mode = FDM_AUTO; // Function Keys (G15-G16)
    divstate[13].click = true;

    divstate[14].mode = FDM_AUTO; // Function Keys (G20-G23)
    divstate[14].click = true;

    arbuf[0].timeout = 500;
    arbuf[0].interval = 1000/30;
    arbuf[1].timeout = 300;
    arbuf[1].interval = 1000/30;
    arbuf[2].timeout = 500;
    arbuf[2].interval = 1000/40;
    arbuf[3].timeout = 300;
    arbuf[3].interval = 1000/40;

    for(i = 0; i < 256; i++)
        keystate[i] = false;
    
    for(i = 0; i < 4; i++)
        ledstate[i] = false;
    
    keyboard_sound(125);
}

static void keyboard_selftest() {
    keyboard_defaults();

    uart_putc_raw(uart0, KBD_FWID); // Firmware ID
    uart_putc_raw(uart0, KBD_HWID); // Hardware ID
    uart_putc_raw(uart0, 0x00);     // No Error
    uart_putc_raw(uart0, 0x00);     // No Keycode
}

static int64_t keyboard_selftest_callback(alarm_id_t id, void *user_data) {
    keyboard_selftest();
    return 0;
}

static void keyboard_parsecmd() {
    int i;

    // Ignore extra-long commands
    if(cmd_pos >= 3) return;

    if(cmd_param[0] & 1) {
        switch(cmd_param[0]) {
            case 0x8B: // Resume Keyboard Transmission
                ledstate[1] = false;
                inhibit = false;
                break;
            case 0x89: // Inhibit Keyboard Transmission
                ledstate[1] = true;
                inhibit = true;
                uart_putc_raw(uart0, KBD_LOCK_ACK);
                break;
            case 0x13: // Turn on LEDs
                for(i = 0; i < 4; i++)
                    if(cmd_param[1] & (1 << i))
                        ledstate[i] = false;
                break;
            case 0x11: // Turn off LEDs
                for(i = 0; i < 4; i++)
                    if(cmd_param[1] & (1 << i))
                        ledstate[i] = true;
                break;
            case 0x99: // Disable Keyclick
                keyclick_volume = 0;
                break;
            case 0x1B: // Enable Keyclick, Set Volume
                keyclick_volume = 7 - (cmd_param[1] & 7);
                break;
            case 0xB9: // Disable Ctrl Keyclick
                ctrlclick = false;
                break;
            case 0xBB: // Enable Ctrl Keyclick
                ctrlclick = true;
                break;
            case 0x9F: // Sound Keyclick
                keyboard_sound(2);
                break;
            case 0xA1: // Disable Bell
                bell_volume = 0;
                break;
            case 0x23: // Enable Bell, Set Volume
                bell_volume = 7 - (cmd_param[1] & 7);
                break;
            case 0xA7: // Sound Bell
                keyboard_sound(125);
                break;
            case 0xC1: // Temporary Auto-Repeat Inhibit
                autoinhibit = 1;
                break;
            case 0xE3: // Enable Auto-Repeat Across Keyboard
                autoinhibit = 0;
                break;
            case 0xE1: // Disable Auto-Repeat Across Keyboard
                autoinhibit = 2;
                break;
            case 0xD9: // Change All Auto-Repeat to Down-Only
                break;
            case 0xAB: // Request Keyboard ID
                uart_putc_raw(uart0, KBD_FWID); // Firmware ID
                uart_putc_raw(uart0, KBD_HWID); // Hardware ID
                break;
            case 0xFD: // Jump to Power-Up
                keyboard_selftest();
                break;
            case 0xCB: // Jump to Test Mode
                break;
            case 0xD3: // Reinstate Defaults
                keyboard_defaults();
                break;
        }
    } else {
        uint div = (cmd_param[0] >> 3) & 0xF;
        uint mode = (cmd_param[0] >> 1) & 0x3;

        if(div == 0x0) {
            // Invalid command
            return;
        }

        if(div == 0xF) {
            // Set Auto-Repeat Buffer Parameters
            return;
        }

        // Select division mode and optionally auto-repeat buffer
        if(cmd_pos) {
            divstate[div].arbuf = cmd_param[1];
        }
        if(mode != 2) {
            divstate[div].mode = mode;
        }
    }
}

static int map_funcdiv( uint8_t s )
{
	switch ( s )
	{
		case 0x92:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D:
		case 0x9E:
		case 0x9F:
		case 0xA1:
		case 0xA2:
		case 0xA3:
		case 0xA4:
			return 2;
		case 0xBC:
			return 3;
		case 0xBD:
		case 0xBE:
			return 4;
		case 0xB0:
		case 0xB1:
			return 5;
		case 0xAF:
		case 0xAE:
			return 6;
		case 0xA7:
		case 0xA8:
			return 7;
		case 0xA9:
		case 0xAA:
			return 8;
		case 0x8A:
		case 0x8B:
		case 0x8C:
		case 0x8D:
		case 0x8E:
		case 0x8F:
			return 9;
		case 0x56:
		case 0x57:
		case 0x58:
		case 0x59:
		case 0x5A:
			return 10;
		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67:
		case 0x68:
			return 11;
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
			return 12;
		case 0x7C:
		case 0x7D:
			return 13;
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
			return 14;
		default:
			return 1;
	}
}

int arcode=-1;
uint8_t dur[10];
int durp=0;
unsigned long arstart;

static void keyboard_scan() {
	static int artimer = 0;
	static int lastar = -1;
	unsigned long nows = board_millis();
	unsigned long arn;
	static int nextar;

	int i = 0;
	int fd, arr= 0, ks=0,dua=0;
	durp=0;
	/* Scan keys */
	for ( i = 0; i < 256; i++ ) {
		fd = map_funcdiv( i );
		if ( keystate[i] && !keys[i] ) {
			keyboard_sound(2);

			if ( divstate[fd].mode == FDM_AUTO ) {
				arcode = i;
				arstart = nows + arbuf[divstate[fd].arbuf].timeout;
			} else {
				ks = 1;
			}

			if ( divstate[fd].mode == FDM_DNUP )
				dua = 1;
			uart_putc_raw(uart0, i );
			keys[i] = true;
		} else if ( keystate[i] && keys[i] ) {
			if ( divstate[fd].mode == FDM_DNUP )
				dua = 1;
		} else if ( keys[i] && !keystate[i] ) {
			if ( arcode == i ) {
				arr = 1;
				arcode = -1;
			}

			if ( divstate[fd].mode == FDM_DNUP ) {
				/* Check for all ups */
				dur[durp++] = i;
			}
			keys[i] = false;
		}
	}
	
	/* If downup released and all ups */
	if ( durp && !dua) {
		uart_putc_raw(uart0, KBD_ALL_UPS );
		ks = 1;
	} else if ( durp ) {
		for ( i = 0; i < durp; i++)
			uart_putc_raw(uart0, dur[i]);
		ks = 1;
	}

	if ( arr ) {
		for ( i = 0; i < 256; i++ ) {
			fd = map_funcdiv( i );
			if (  (divstate[fd].mode == FDM_AUTO) && keys[i] ) {
				arcode = i;
				arstart = nows + arbuf[divstate[fd].arbuf].timeout;
			}
		}
	}

	if ( nows > arstart ) {
		if ( arcode == -1 )
			return;
			
		fd = map_funcdiv(arcode);
		arn = nows - arstart;
		arr = (arn / (unsigned long) arbuf[divstate[fd].arbuf].interval);
		if ( nextar != arr ) {
		//	Serial.print("Repeat for ");
		//	Serial.println(arcode);
			nextar = arr;
			if ( ks || ! arr ) 
				uart_putc_raw(uart0, arcode );
			else
				uart_putc_raw(uart0, KBD_METRONOME );
		} 
	}

}

void keyboard_dowork() {
    if(uart_is_readable(uart0)) {
        cmd_param[cmd_pos] = uart_getc(uart0);
        if(cmd_param[cmd_pos] & 0x80) {
            keyboard_parsecmd();
            cmd_pos = 0;
        } else {
            if(cmd_pos < 3)
                cmd_pos++;
        }
    }
    
    keyboard_scan();
}

void keyboard_init() {
    pwm_config pwmcfg;

    uart_init(uart0, 4800);
    uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
    
    // Bell / Keyclick output
    gpio_set_function(10, GPIO_FUNC_PWM);
    audio_slice = pwm_gpio_to_slice_num(10);
    audio_chan = pwm_gpio_to_channel(10);
    pwmcfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&pwmcfg, 125);
    pwm_init(audio_slice, &pwmcfg, false);
    pwm_set_wrap(audio_slice, 500);
    pwm_set_chan_level(audio_slice, audio_chan, 0);
    pwm_set_enabled(audio_slice, true);

    arstart = board_millis() + 99999999;

    sleep_ms(50);
    keyboard_selftest();
}
