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

## Requisitos

Picotool faz uma bruxaria e consegue fazer funcionar, portanto siga os passo nos docs de raspberrypi/picotool e compila o picotool (se no mac, não testei em outros).

```git submodule update --init --recursive --force``` para atualizar os submódulos

Coloque pico-sdk/ e picotool/ dentro ~/pico e defina as env no PATH (insira em .zshrc para manter entre sessões)

```bash
export PATH="$HOME/.local/bin:$PATH"
export PICO_SDK_PATH="$HOME/pico/pico-sdk"
```

## Configurar as placas

Adicionar dentro da pasta do pico-sdk: ```$HOME/pico/pico-sdk/src/boards/include/boards``` os arquivos inclusos em boards

## Configurar com o clangd

### MacOS

1. Baixar ArmGNUToolchain (recomendado pelo site)
2. Fazer um link simbólico para o compile_commands.json
   ```ln -s build/compile_commands.json .```
3. Crie o arquivo ```.clangd``` e copie os diretórios em ```/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/arm-none-eabi-g++ -E -x c++ - -v < /dev/null```:
   #include "..." search starts here:
   #include <...> search starts here:
e cole no arquivo:

  ```yaml
  CompileFlags:
    Add:
      - -isystem
      - /path/from/command
  ```
