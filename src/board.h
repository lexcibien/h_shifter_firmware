#include <cstdint>

// Waveshare RP2040 Zero
// https://www.waveshare.com/wiki/RP2040-Zero
// https://www.waveshare.com/w/upload/4/4c/RP2040_Zero.pdf
// https://www.waveshare.com/img/devkit/RP2040-Zero/RP2040-Zero-details-7.jpg
//
// https://www.mischianti.org/2022/09/19/waveshare-rp2040-zero-high-resolution-pinout-and-specs/
// https://www.mischianti.org/wp-content/uploads/2022/09/Waveshare-rp2040-zero-Raspberry-Pi-Pico-alternative-pinout.jpg

/*
                Pin#              Pin#
                    ___(_____)___
              5v 1 |   *USB C*   | 23 GPIO0
             GND 2 |             | 22 GPIO1
            3.3v 3 |             | 21 GPIO2
          GPIO29 4 |             | 20 GPIO3
          GPIO28 5 |             | 19 GPIO4
          GPIO27 6 |             | 18 GPIO5
          GPIO26 7 |             | 17 GPIO6
          GPIO15 8 |             | 16 GPIO7
          GPIO14 9 |__|_|_|_|_|__| 15 GPIO8
                      1 1 1 1 1
                      0 1 2 3 4

                    Pin10 = GPIO13
                    Pin11 = GPIO12
                    Pin12 = GPIO11
                    Pin13 = GPIO10
                    Pin14 = GPIO9
*/

// Fim de curso do H-shifter - Vista de cima com a primeira marcha topo esquerda
const uint8_t SW_FRONT = 2;
const uint8_t SW_BACK = 3;
const uint8_t SW_LEFT = 4;
const uint8_t SW_RIGHT = 5;
const uint8_t SW_REVERSE = 6;

// Botões da manopla de caminhão
const uint8_t SW_KNOB_RANGE = 26; // -> Escolher um pino entre 26 ao 29 (precisa ser adc!)
const uint8_t SW_KNOB_SPLIT = 8;
const uint8_t BTN_KNOB_ENGINE_BRAKE = 9;

const uint8_t SW_ENABLE_SEQUENTIAL = 10;
