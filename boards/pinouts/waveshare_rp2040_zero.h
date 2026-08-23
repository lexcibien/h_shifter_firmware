/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_WAVESHARE_RP2040_ZERO_H
#define _BOARDS_WAVESHARE_RP2040_ZERO_H

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

pico_board_cmake_set(PICO_PLATFORM, rp2040)

// For board detection
#define WAVESHARE_RP2040_ZERO

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

// --- WS2812 ---
#ifndef PICO_DEFAULT_WS2812_PIN
#define PICO_DEFAULT_WS2812_PIN 16
#endif

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 1
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 6
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 7
#endif

// --- SPI ---
#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 1
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 10
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 11
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 12
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 13
#endif

// --- FLASH ---
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 4
#endif

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (2 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif
// All boards have B1 RP2040
#ifndef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED  0
#endif

#endif
