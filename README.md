# Digital Equipment LK201 Keyboard & VS-XXX-AA Mouse Emulator

Hardware:
- Adafruit ItsyBitsy RP2040 board, although almost any RP2040 board should work.
- MAX3233 or other 3.3v to RS232 Level shifter with two RX/TX pairs
- USB OTG Adapter
- Optional USB (Full Speed) Hub
- USB Keyboard & Mouse (Tested with Logitech K400+)

GPIO0 UART0 Keyboard -> VAX
GPIO1 UART0 Keyboard <- VAX
GPIO8 UART1 Mouse -> VAX
GPIO9 UART1 Mouse <- VAX
GPIO10 Piezo Buzzer for Keyclick & Bell
GPIO11 Board LED (Adafruit ItsyBitsy RP2040)

Connect Ground and +5V power to the appropriate pins on the DEC Mouse connector.
Be aware that +/-12V are present on the mouse and keyboard connectors!
You may need to connect the DEC Mouse pin 7 to ground to tell the host computer/terminal a mouse is connected.

The foundations are in place to support the DEC Tablet protocol, but a TinyUSB driver would need to be written for the USB Tablet.
