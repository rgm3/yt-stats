# YouTube Subscriber Counter

# Components

* [Adafruit Feather HUZZAH with ESP8266][esp]
* [20 mm Round Big LED][led]
* WaveShare 2.9" 296x128 3-color b/w/r e-Paper module
* [YoutubeApi library for Arduino][ytapilib]

[esp]: https://www.adafruit.com/product/2821
[led]: https://www.radioshack.com/products/radioshack-20-0mm-round-big-red-led?variant=20332057541
[ytapilib]: https://github.com/witnessmenow/arduino-youtube-api

# Building

The build is specific to the ESP8266 module.

```bash
arduino-cli lib install ArduinoJson
arduino-cli lib install Effortless-SPIFFS
arduino-cli lib install YoutubeApi
arduino-cli lib install GxEPD2
arduino-cli lib install WiFiManager

make
```

# Waveshare module hardware connections

These pins on the Adafruit Feather HUZZAH with ESP8266 were chosen because they're
mostly near one another on the same side of the board, which made prototyping easier.

| ePaper | Huzzah Pin     |
|--------|----------------|
| BUSY   | D5 (any GPIO)  |
| RST    | D2 (any GPIO)  |
| DC     | D16 (any GPIO) |
| CS     | D15            |
| CLK    | D14 (SCK)      |
| DIN    | D13 (MOSI)     |
| GND    | GND            |
| 3.3V   | 3V3            |

# Convert images for use with ePaper module

See: https://learn.adafruit.com/preparing-graphics-for-e-ink-displays?view=all

```bash
convert input.jpg \
  -dither FloydSteinberg \
  -define dither:diffusion-amount=85% \
  -remap gfx/eink-3color.png \
  -type truecolor \
  output.bmp
```

# Links

* https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/pinouts
* https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/using-arduino-ide
* https://www.waveshare.com/wiki/2.9inch_e-Paper_Module_(B)
* https://github.com/waveshare/e-Paper
* https://github.com/witnessmenow/arduino-youtube-api
* https://github.com/tzapu/WiFiManager
* https://github.com/thebigpotatoe/Effortless-SPIFFS
