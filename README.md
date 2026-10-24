# Firmware H Shifter

Firmware para o câmbio H com uma placa Waveshare RP2040 Zero. A placa apresenta
12 botões virtuais ao computador como um joystick.

## Requisitos

- CMake 3.35 ou superior.
- Raspberry Pi Pico SDK 2.3.0 ou superior, com os submódulos inicializados.
- Toolchain GNU para ARM (`arm-none-eabi-gcc`) disponível no `PATH`.
- Python disponível como `python` no `PATH` (usado para gerar o relatório de memória durante a compilação).
- `picotool` disponível no `PATH` para gravar o firmware pelo alvo `flash`.

Instale o Pico SDK e o `picotool` seguindo as instruções dos respectivos
projetos: [pico-sdk](https://github.com/raspberrypi/pico-sdk) e
[picotool](https://github.com/raspberrypi/picotool). Por exemplo, para manter
o SDK em `~/pico/pico-sdk`:

```bash
mkdir -p "$HOME/pico"
git clone https://github.com/raspberrypi/pico-sdk.git "$HOME/pico/pico-sdk"
git -C "$HOME/pico/pico-sdk" submodule update --init --recursive
export PICO_SDK_PATH="$HOME/pico/pico-sdk"
```

Configure `PICO_SDK_PATH` no arquivo de inicialização do seu shell se quiser
mantê-lo definido entre sessões. Instale também a toolchain ARM e o `picotool`
conforme as instruções oficiais para o seu sistema operacional.

## Compilar

Na raiz do repositório, configure o projeto e compile:

```bash
cmake -S . -B build
cmake --build build --parallel
```

Se possui a extensão no VSCode CMake Tools é mais fácil.

O CMake seleciona a placa `waveshare_rp2040_zero`. A compilação gera, entre
outros arquivos, `build/H_SHIFTER_JAPATECH.uf2` e
`build/H_SHIFTER_JAPATECH.bin`. Também é possível executar os dois comandos
novamente depois de alterações para recompilar.

## Gravar o firmware na placa

Conecte a RP2040 Zero ao computador por USB em modo BOOTSEL: desconecte a placa,
segure o botão **BOOT** enquanto conecta o cabo USB e solte o botão quando ela
for reconhecida. Na raiz do repositório, execute:

```bash
cmake --build build --target flash
```

Esse alvo compila o firmware, se necessário, e chama `picotool` para gravar o
arquivo `.bin` na flash da placa e iniciá-lo. Ao terminar, a placa reinicia e
passa a executar o firmware.

Como alternativa, se a placa aparecer como uma unidade USB `RPI-RP2`, copie
`build/H_SHIFTER_JAPATECH.uf2` para essa unidade. Ela reiniciará após a cópia.
