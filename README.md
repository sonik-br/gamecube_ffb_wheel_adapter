# gamecube_ffb_wheel_adapter

A DIY adapter that can receive input from a logitech wheel and output as an GameCube Speed Force wheel with Force FeedBack.

This is currently untested as I do not have hardware to try on.
Was being developed and tested on a gamecube but my console does not read discs anymore and my wii does not work.
I did tested it months ago with F-Zero GX and it was working as expected.

## Building

Requires a Raspberry Pi Pico (RP2040) board, USB OTG adapter and a male GC connector.

| GameCube | Pico |
|----------|------|
| 5V       | VBUS |
| GND      | GND  |
| Data     | GP 0 |
| 3.3V     | GP 1 |

USB wheel should connect to pico using a USB OTG adapter.

Joybus (gamecube) pins can be customized.
The 3.3v connection is optional. It can be used to detect if the device is connected to a GC console or to a PC
If not wired, must comment out the code that handles it.

Firmware builds under Arduino IDE.

Required libs. Install using Arduino IDE.

[arduino-pico (3.9.4)](https://github.com/earlephilhower/arduino-pico#installing-via-arduino-boards-manager)<br/>
[Pico-PIO-USB (0.5.3)](https://github.com/sekigon-gonnoc/Pico-PIO-USB)<br/>
[Adafruit_TinyUSB_Arduino (3.3.4)](https://github.com/adafruit/Adafruit_TinyUSB_Arduino)

Configure IDE as:
* Board: Raspberry Pi Pico
* CPU Speed: 133MHz
* USB Stack: Adafruit TinyUSB host (native)

## Credits

Logitech USB input code from my other project.
[lgff_wheel_adapter](https://github.com/sonik-br/lgff_wheel_adapter)

[JonnyHaystack](https://github.com/JonnyHaystack/joybus-pio) and [RobertDaleSmith](https://github.com/RobertDaleSmith/joybus-pio) for joybus-pio.

Xbox (XINPUT) driver by [Ryzee119](https://github.com/Ryzee119/tusb_xinput).

## Disclaimer
Code and wiring directions are provided to you 'as is' and without any warranties. Use at your own risk.