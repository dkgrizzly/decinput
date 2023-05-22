/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021, Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "bsp/board.h"
#include "tusb.h"
#include "keyboard.h"
#include "mouse.h"

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

#define MAX_REPORT  4

static uint8_t const keycode2dec[256] = { 
    0x00, /* 0x00 NONE */
    0x00, /* 0x01 ERR ROLLOVER */
    0x00, /* 0x02 POST FAIL */
    0x00, /* 0x03 UNDEFINED */
    0xC2, /* 0x04 'A' */
    0xD9, /* 0x05 'B' */
    0xCE, /* 0x06 'C' */
    0xCD, /* 0x07 'D' */
    0xCC, /* 0x08 'E' */
    0xD2, /* 0x09 'F' */
    0xD8, /* 0x0a 'G' */
    0xDD, /* 0x0b 'H' */
    0xE6, /* 0x0c 'I' */
    0xE2, /* 0x0d 'J' */
    0xE7, /* 0x0e 'K' */
    0xEC, /* 0x0f 'L' */
    0xE3, /* 0x10 'M' */
    0xDE, /* 0x11 'N' */
    0xEB, /* 0x12 'O' */
    0xF0, /* 0x13 'P' */
    0xC1, /* 0x14 'Q' */
    0xD1, /* 0x15 'R' */
    0xC7, /* 0x16 'S' */
    0xD7, /* 0x17 'T' */
    0xE1, /* 0x18 'U' */
    0xD3, /* 0x19 'V' */
    0xC6, /* 0x1a 'W' */
    0xC8, /* 0x1b 'X' */
    0xDC, /* 0x1c 'Y' */
    0xC3, /* 0x1d 'Z' */
    0xC0, /* 0x1e '!' */
    0xC5, /* 0x1f '@' */
    0xCB, /* 0x20 '#' */
    0xD0, /* 0x21 '$' */
    0xD6, /* 0x22 '%' */
    0xDB, /* 0x23 '^' */
    0xE0, /* 0x24 '&' */
    0xE5, /* 0x25 '*' */
    0xEA, /* 0x26 '(' */
    0xEF, /* 0x27 ')' */
    0xBD, /* 0x28 CR  */
    0x71, /* 0x29 ESC */
    0xBC, /* 0x2a BS  */
    0xBE, /* 0x2b Tab */
    0xD4, /* 0x2c ' ' */
    0xF9, /* 0x2d '_' */
    0xF5, /* 0x2e '+' */
    0xFA, /* 0x2f '{' */
    0xF6, /* 0x30 '}' */
    0xF7, /* 0x31 '|' */
    0xBF, /* 0x32 '#' */
    0xF2, /* 0x33 ';' */
    0xE8, /* 0x34 ''' */
    0xBF, /* 0x35 '`' */
    0xFB, /* 0x36 ',' */
    0xED, /* 0x37 '.' */
    0xF3, /* 0x38 '/' */
    0x00, /* 0x39 CAPS */
    0x56, /* 0x3a F1 */
    0x57, /* 0x3b F2 */
    0x58, /* 0x3c F3 */
    0x59, /* 0x3d F4 */
    0x5A, /* 0x3e F5 */
    0x64, /* 0x3f F6 */
    0x65, /* 0x40 F7 */
    0x66, /* 0x41 F8 */
    0x67, /* 0x42 F9 */
    0x68, /* 0x43 F10 */
    0x71, /* 0x44 F11 */
    0x72, /* 0x45 F12 */
    0x7C, /* 0x46 SysRq */
    0x7D, /* 0x47 Scroll */
    0xB0, /* 0x48 Pause */
    0x8B, /* 0x49 INS */
    0x8A, /* 0x4a HOME */
    0x8E, /* 0x4b PGUP */
    0x8C, /* 0x4c DEL */
    0x8D, /* 0x4d END */
    0x8F, /* 0x4e PGDN */
    0xA8, /* 0x4f RIGHT */
    0xA7, /* 0x50 LEFT */
    0xA9, /* 0x51 DOWN */
    0xAA, /* 0x52 UP */
    0xA1, /* 0x53 NUMLOCK */
    0xA2, /* 0x54 '/' */
    0xA3, /* 0x55 '*' */
    0xA4, /* 0x56 '-' */
    0x9C, /* 0x57 '+' */
    0x95, /* 0x58 ENT */
    0x96, /* 0x59 '1' */
    0x97, /* 0x5a '2' */
    0x98, /* 0x5b '3' */
    0x99, /* 0x5c '4' */
    0x9A, /* 0x5d '5' */
    0x9B, /* 0x5e '6' */
    0x9D, /* 0x5f '7' */
    0x9E, /* 0x60 '8' */
    0x9F, /* 0x61 '9' */
    0x92, /* 0x62 '0' */
    0x94, /* 0x63 '.' */
    0xF7, /* 0x64 '|' */
    0xB1, /* 0x65 COMPOSE */
    0x00, /* 0x66 POWER */
    0xF5, /* 0x67 '=' */
    0x73, /* 0x68 F13 */
    0x74, /* 0x69 F14 */
    0x7C, /* 0x6A F15 */
    0x7D, /* 0x6B F16 */
    0x80, /* 0x6C F17 */
    0x81, /* 0x6D F18 */
    0x82, /* 0x6E F19 */
    0x83, /* 0x6F F20 */
    0xA1, /* 0x70 F21 */
    0xA2, /* 0x71 F22 */
    0xA3, /* 0x72 F23 */
    0xA4, /* 0x73 F24 */
    0x7D, /* 0x74 Open */
    0x7C, /* 0x75 Help */
    0x74, /* 0x76 Menu */
    0x8D, /* 0x77 Select */
    0x00, /* 0x78 Stop */
    0x00, /* 0x79 Again */
    0x00, /* 0x7a Undo */
    0x00, /* 0x7b Cut */
    0x00, /* 0x7c Copy */
    0x00, /* 0x7d Paste */
    0x8A, /* 0x7e Find */
    0x00, /* 0x7f Mute */
};



// Each HID instance can has multiple reports
static struct
{
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
}hid_info[CFG_TUH_HID];

static void process_kbd_report(hid_keyboard_report_t const *report);
static void process_mouse_report(hid_mouse_report_t const * report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

void hid_app_task(void)
{
  // nothing to do
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  //printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
  keyboard_sound(125);

  // Interface protocol (hid_interface_protocol_enum_t)
  const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  //printf("HID Interface Protocol = %s\r\n", protocol_str[itf_protocol]);

  // By default host stack will use activate boot protocol on supported interface.
  // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
  if ( itf_protocol == HID_ITF_PROTOCOL_NONE )
  {
    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    //printf("HID has %u reports \r\n", hid_info[instance].report_count);
  }

  // request to receive report
  // tuh_hid_report_received_cb() will be invoked when report is available
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    //printf("Error: cannot request to receive report\r\n");
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  keyboard_sound(125);
  //printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch (itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:
      //TU_LOG2("HID receive boot keyboard report\r\n");
      process_kbd_report( (hid_keyboard_report_t const*) report );
    break;

    case HID_ITF_PROTOCOL_MOUSE:
      //TU_LOG2("HID receive boot mouse report\r\n");
      process_mouse_report( (hid_mouse_report_t const*) report );
    break;

    default:
      // Generic report requires matching ReportID and contents with previous parsed report info
      process_generic_report(dev_addr, instance, report, len);
    break;
  }

  // continue to request to receive report
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    //printf("Error: cannot request to receive report\r\n");
  }
}

//--------------------------------------------------------------------+
// Keyboard
//--------------------------------------------------------------------+

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode)
{
  for(uint8_t i=0; i<6; i++)
  {
    if (report->keycode[i] == keycode)  return true;
  }

  return false;
}

static void process_kbd_report(hid_keyboard_report_t const *report)
{
  static hid_keyboard_report_t prev_report = { 0, 0, {0} }; // previous report to check key released

  // Modifier Keys
  keystate[0xAE] = (report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT)) ? 1 : 0;
  keystate[0xAF] = (report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL)) ? 1 : 0;
  keystate[0xB1] = (report->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT)) ? 1 : 0;
  //keystate[0xB2] = (report->modifier & (KEYBOARD_MODIFIER_LEFTMETA | KEYBOARD_MODIFIER_RIGHTMETA)) ? 1 : 0;

  for(uint8_t i=0; i<6; i++) {
    if ( prev_report.keycode[i] ) {
      if ( !find_key_in_report(report, prev_report.keycode[i]) ) {
        if(keycode2dec[prev_report.keycode[i]] != 0)
            keystate[keycode2dec[prev_report.keycode[i]]] = 0;
      }
    }
    if ( report->keycode[i] ) {
      if ( !find_key_in_report(&prev_report, report->keycode[i]) ) {
        if(keycode2dec[report->keycode[i]] != 0)
            keystate[keycode2dec[report->keycode[i]]] = 1;

        // not existed in previous report means the current key is newly pressed
        //bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
        //uint8_t ch = keycode2ascii[report->keycode[i]][is_shift ? 1 : 0];
        //putchar(ch);
        //if ( ch == '\r' ) putchar('\n'); // added new line for enter key

        //fflush(stdout); // flush right away, else nanolib will wait for newline
      }
    }
  }

  prev_report = *report;
}

//--------------------------------------------------------------------+
// Mouse
//--------------------------------------------------------------------+

static void process_mouse_report(hid_mouse_report_t const * report)
{
  static hid_mouse_report_t prev_report = { 0 };

  //------------- button state  -------------//
  uint8_t buttons = 0;

  if(report->buttons & MOUSE_BUTTON_LEFT)
      buttons |= 0x04;

  if(report->buttons & MOUSE_BUTTON_MIDDLE)
      buttons |= 0x02;

  if(report->buttons & MOUSE_BUTTON_RIGHT)
      buttons |= 0x01;

  //------------- cursor movement -------------//
  mouse_packet_enqueue(report->x, report->y, buttons);
}

//--------------------------------------------------------------------+
// Generic Report
//--------------------------------------------------------------------+
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) dev_addr;

  uint8_t const rpt_count = hid_info[instance].report_count;
  tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
  tuh_hid_report_info_t* rpt_info = NULL;

  if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0)
  {
    // Simple report without report ID as 1st byte
    rpt_info = &rpt_info_arr[0];
  }else
  {
    // Composite report, 1st byte is report ID, data starts from 2nd byte
    uint8_t const rpt_id = report[0];

    // Find report id in the array
    for(uint8_t i=0; i<rpt_count; i++)
    {
      if (rpt_id == rpt_info_arr[i].report_id )
      {
        rpt_info = &rpt_info_arr[i];
        break;
      }
    }

    report++;
    len--;
  }

  if (!rpt_info)
  {
    //printf("Couldn't find the report info for this report !\r\n");
    return;
  }

  // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
  // - Keyboard                     : Desktop, Keyboard
  // - Mouse                        : Desktop, Mouse
  // - Gamepad                      : Desktop, Gamepad
  // - Consumer Control (Media Key) : Consumer, Consumer Control
  // - System Control (Power key)   : Desktop, System Control
  // - Generic (vendor)             : 0xFFxx, xx
  if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP )
  {
    switch (rpt_info->usage)
    {
      case HID_USAGE_DESKTOP_KEYBOARD:
        //TU_LOG1("HID receive keyboard report\r\n");
        // Assume keyboard follow boot report layout
        process_kbd_report( (hid_keyboard_report_t const*) report );
      break;

      case HID_USAGE_DESKTOP_MOUSE:
        //TU_LOG1("HID receive mouse report\r\n");
        // Assume mouse follow boot report layout
        process_mouse_report( (hid_mouse_report_t const*) report );
      break;

      default: break;
    }
  }
}
