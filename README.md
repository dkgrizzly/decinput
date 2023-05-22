# Digital Equipment LK201 Keyboard & VS-XXX-AA Mouse Emulator

Parts:
- Adafruit ItsyBitsy RP2040 board, although almost any RP2040 board should work.
- MAX3233 or other 3.3v to RS232 Level shifter with two RX/TX pairs
- USB OTG Adapter
- Optional USB (Full Speed) Hub
- USB Keyboard & Mouse (Tested with Logitech K400+)

Connections:
+ GPIO0 UART0 Keyboard -> VAX Keyboard Port Pin 4 / Alpha Pin 3
+ GPIO1 UART0 Keyboard <- VAX Keyboard Port Pin 1 / Alpha Pin 2
+ GPIO8 UART1 Mouse -> VAX Mouse Port Pin 2 / Alpha Pin 6
+ GPIO9 UART1 Mouse <- VAX Mouse Port Pin 3 / Alpha Pin 7
+ GPIO10 Piezo Buzzer for Keyclick & Bell
+ GPIO11 Board LED (Adafruit ItsyBitsy RP2040)
+ Ground - VAX Mouse Port Pin 1 / Alpha Pins 1,5,8,9,15
+ +5V - VAX Mouse Port Pin 5 / Alpha Pin 13

Be aware that +/-12V are present on the mouse and keyboard connectors!
You may need to connect the DEC Mouse pin 7 to ground to tell the host computer/terminal a mouse is connected.

The foundations are in place to support the DEC Tablet protocol, but a TinyUSB driver would need to be written for the USB Tablet.
