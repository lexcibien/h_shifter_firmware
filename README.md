# Firmware H Shifter

Firmware para o câmbio H com uma placa Waveshare RP2040 Zero. A placa apresenta
12 botões virtuais ao computador como um joystick.

## Configurador WebUSB

O firmware mantém o joystick HID e também expõe uma interface WebUSB para
configuração. Abra `https://lexcibien.github.io/h_shifter_firmware/` no Chrome
ou Edge, pressione **Conectar dispositivo** e escolha o DAZ Shifter.

As leituras de diagnóstico ficam disponíveis após conectar. Para liberar
gravação, restauração, redetecção da manopla e reinício:

- V1: segure o botão de modo sequencial e o switch direito por um segundo;
- V2: coloque a chave em sequencial e segure o switch direito por um segundo.

O LED fica verde brevemente quando a configuração é liberada. O botão
**Salvar na flash** grava em dois setores alternados e as opções persistem
depois de desligar ou atualizar o firmware. O modo inicial configurável é
exclusivo da V1; na V2 a chave física sempre determina o modo.

No Linux pode ser necessário instalar `assets/99-daz-shifter.rules` em
`/etc/udev/rules.d/`, recarregar as regras do udev e reconectar o dispositivo.
WebUSB exige HTTPS e não é suportado pelo Firefox ou Safari.

Para publicar um fork em outro endereço, configure o descritor antes de
compilar, omitindo o prefixo `https://`:

```bash
cmake --preset waveshare_rp2040_zero_v1 \
  -DH_SHIFTER_WEBUSB_URL=usuario.github.io/repositorio/
```

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

Na raiz do repositório, escolha o preset correspondente ao projeto e compile:

```bash
# Projeto V1 (12 botões virtuais)
cmake --preset waveshare_rp2040_zero_v1
cmake --build --preset waveshare_rp2040_zero_v1

# Projeto V2 (13 botões virtuais)
cmake --preset waveshare_rp2040_zero_v2
cmake --build --preset waveshare_rp2040_zero_v2
```

No VS Code com a extensão CMake Tools, execute `CMake: Select Configure Preset`
e escolha `Waveshare RP2040 Zero - V1` ou `Waveshare RP2040 Zero - V2`. A
extensão guarda o preset escolhido para o workspace e usa o diretório de build
correspondente.

Se `SHIFTER_PROJECT_VERSION` não for informado, o CMake usa
`SHIFTER_PROJECT_V1`. Se possui a extensão no VSCode CMake Tools é mais
fácil.

Os dois presets selecionam a placa física `waveshare_rp2040_zero` e configuram
a versão da lógica e o nome USB (`DAZ Shifter V1` ou `DAZ Shifter V2`). Os
arquivos são gerados em `build/<preset>/`, incluindo
`H_SHIFTER_JAPATECH.uf2` e `H_SHIFTER_JAPATECH.bin`.

### Formatação e análise estática

O CMake disponibiliza targets para as mesmas verificações de qualidade usadas
no fluxo anterior:

```bash
# Formatar os arquivos C/C++ do projeto
cmake --build --preset waveshare_rp2040_zero_v1 --target format

# Verificar sem alterar arquivos
cmake --build --preset waveshare_rp2040_zero_v1 --target format-check

# Compilar o firmware e executar clang-tidy usando o compile_commands.json
cmake --build --preset waveshare_rp2040_zero_v1 --target lint
```

Os targets `format` e `format-check` usam o `.clang-format` da raiz. O target
`lint` usa o `.clang-tidy` da raiz e o banco de compilação gerado pelo CMake,
incluindo os defines e headers do pico-sdk para a placa e a versão escolhidas.

Para executar `clang-tidy` automaticamente em cada compilação das fontes do
projeto, configure com `-DH_SHIFTER_ENABLE_CLANG_TIDY=ON`.

## Executar os testes

Depois de configurar o projeto, execute os testes a partir da raiz do
repositório usando o mesmo preset da versão desejada:

```bash
# Projeto V1
ctest --preset waveshare_rp2040_zero_v1

# Projeto V2
ctest --preset waveshare_rp2040_zero_v2
```

É possível usar cmake com um target test também:

```bash
# Projeto V1
cmake --build --preset waveshare_rp2040_zero_v1 --target test

# Projeto V2
cmake --build --preset waveshare_rp2040_zero_v2 --target test
```

### Logs e diagnóstico

Por padrão, o firmware expõe somente a interface HID. Para habilitar a porta
USB CDC e os logs operacionais padronizados, configure com:

```bash
cmake --preset waveshare_rp2040_zero_v1 -DH_SHIFTER_LOG=ON
cmake --build --preset waveshare_rp2040_zero_v1
```

Os logs operacionais registram inicialização, configuração, montagem USB e
mudanças entre os modos H e sequencial. Para incluir também o diagnóstico
detalhado de entradas e saídas, defina `H_SHIFTER_DEBUG` durante a compilação:

```bash
cmake --preset waveshare_rp2040_zero_v1 \
  -DH_SHIFTER_LOG=ON \
  -DCMAKE_CXX_FLAGS=-DH_SHIFTER_DEBUG
cmake --build --preset waveshare_rp2040_zero_v1
```

O diagnóstico analógico pode ser incluído adicionando também
`-DDEBUG_ANALOG`. Ele exige `H_SHIFTER_DEBUG` e `H_SHIFTER_LOG=ON`.

## Gravar o firmware na placa

Conecte a RP2040 Zero ao computador por USB em modo BOOTSEL: desconecte a placa,
segure o botão **BOOT** enquanto conecta o cabo USB e solte o botão quando ela
for reconhecida. Na raiz do repositório, execute:

```bash
cmake --build --preset waveshare_rp2040_zero_v1 --target flash
```

Esse alvo compila o firmware, se necessário, e chama `picotool` para gravar o
arquivo `.bin` na flash da placa e iniciá-lo. Ao terminar, a placa reinicia e
passa a executar o firmware.

Como alternativa, se a placa aparecer como uma unidade USB `RPI-RP2`, copie
o arquivo `build/<preset>/H_SHIFTER_JAPATECH.uf2` para essa unidade. Ela
reiniciará após a cópia.
