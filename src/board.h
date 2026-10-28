#pragma once

#include <cstdint>

// Versões de hardware/logica disponíveis. Estes valores precisam ser macros
// porque a seleção acontece durante a compilação com diretivas #if.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SHIFTER_PROJECT_V1 1
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SHIFTER_PROJECT_V2 2

// O ambiente selecionado no PlatformIO define a versão do projeto.
#ifndef SHIFTER_PROJECT_VERSION
#error "Select a versioned PlatformIO environment: waveshare_rp2040_zero_v1 or waveshare_rp2040_zero_v2"
#elif SHIFTER_PROJECT_VERSION != SHIFTER_PROJECT_V1 && SHIFTER_PROJECT_VERSION != SHIFTER_PROJECT_V2
#error "SHIFTER_PROJECT_VERSION must be SHIFTER_PROJECT_V1 or SHIFTER_PROJECT_V2"
#endif

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

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1

/*
  DAZ Projects SIM RACING SHIFTER - logica do projeto V1
  ------------------------------------------------------
  Hardware no RP2040 Zero:
    - GPIOs 2, 3, 4, 5, 6 e 10 ligados a chaves que fecham em GND
    - GPIO 10 pressionado por 1 segundo alterna entre H e sequencial

  Modo H:
    - Marcha 1: GPIO 3 + 5
    - Marcha 2: GPIO 2 + 5
    - Marcha 3: GPIO 3
    - Marcha 4: GPIO 2
    - Marcha 5: GPIO 3 + 4
    - Marcha 6: GPIO 2 + 4
    - Re:       GPIO 3 + 5 + 6

  Modo sequencial:
    - GPIO 3: subir marcha
    - GPIO 2: reduzir marcha
*/

const uint8_t SW_FRONT = 2;
const uint8_t SW_BACK = 3;
const uint8_t SW_LEFT = 4;
const uint8_t SW_RIGHT = 5;
const uint8_t SW_REVERSE = 6;
const uint8_t SW_ENABLE_SEQUENTIAL = 10;

#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2

/*
  DAZ Projects SIM RACING SHIFTER - logica do projeto V2
  ------------------------------------------------------
  Hardware no RP2040 Zero:
    - GPIOs 2, 3, 4, 5 e 6 ligados a chaves que fecham em GND
    - GPIO 2 fechado: modo H; aberto: modo sequencial

  Modo H:
    - Marcha 1: GPIO 3 + 4
    - Marcha 2: GPIO 6 + 4
    - Marcha 3: GPIO 3
    - Marcha 4: GPIO 6
    - Marcha 5: GPIO 3 + 5
    - Marcha 6: GPIO 6 + 5
    - Marcha 7: GPIO 3 + 4 + 5
    - Re:       GPIO 6 + 4 + 5

  Modo sequencial:
    - GPIO 6: subir marcha (botão virtual 9)
    - GPIO 3: reduzir marcha (botão virtual 10)

  O sketch original para Pro Micro foi adaptado para compartilhar o HID
  TinyUSB e a arquitetura modular do projeto V1.
*/

const uint8_t PIN_MODE = 2;
const uint8_t PIN_3 = 3;
const uint8_t PIN_4 = 4;
const uint8_t PIN_5 = 5;
const uint8_t PIN_6 = 6;

#endif

/*
  Manopla de caminhão - compartilhada pelos projetos V1 e V2
  -----------------------------------------------------------
  - GPIO 26 (ADC): seletor de faixa (range) e detecção da manopla
  - GPIO 8: seletor de divisão (split)
  - GPIO 9: freio motor
*/
const uint8_t SW_KNOB_RANGE = 26; // Precisa ser um GPIO com ADC (26 a 29).
const uint8_t SW_KNOB_SPLIT = 8;
const uint8_t BTN_KNOB_ENGINE_BRAKE = 9;
