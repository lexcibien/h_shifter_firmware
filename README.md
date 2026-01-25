# Firmware H Shifter

## Compilar branch tools-customs

Para compilar esse branch, são necessários alguns passos:

1. Baixar o [Compilador GCC para AVR](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers) e salvar em `.pio/packages/toolchain-atmelavr-custom`
2. Adicionar um arquivo `package.json` na pasta

    ```json
    {
      "name": "toolchain-atmelavr",
      "version": "1.0.0",
      "description": "GCC Toolchain for Microchip AVR microcontrollers",
      "keywords": [
        "toolchain",
        "build tools",
        "compiler",
        "assembler",
        "linker",
        "preprocessor",
        "microchip",
        "avr"
      ],
      "homepage": "<https://gcc.gnu.org/wiki/avr-gcc>",
      "license": "GPL-2.0-or-later",
      "system": [
        "linux_x86_64"
      ]
    }
    ```

3. Para ter a nova versão do `ArduinoCore-avr`, deve clonar o repositório na pasta `.pio/packages/framework-arduino-avr-custom`
4. Adicionar um arquivo `package.json` na pasta

    ```json
    {
    "name": "framework-arduino-avr",
    "version": "5.3.0",
    "description": "The official Arduino Wiring-based Framework for Microchip AVR microcontrollers",
    "keywords": [
      "framework",
      "arduino",
      "microchip",
      "avr"
    ],
    "homepage": "https://www.arduino.cc/reference/en",
    "license": "LGPL-2.1-or-later",
    "repository": {
      "type": "git",
      "url": "https://github.com/arduino/ArduinoCore-avr"
      }
    }
    ```

Depois disso irá compilar sem erros.
