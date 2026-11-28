# Configurador DAZ Shifter

O configurador usa Rust e [GPUI Kit 0.11](https://gpui-kit.com/docs/) com o tema Default Dark. A janela reúne as informações do dispositivo, a
configuração de marcha e entradas, as cores dos LEDs e o diagnóstico em tempo real.

## Executar

Use Rust 1.92 ou mais recente e uma sessão gráfica. No Linux, instale as dependências do [GPUI Kit](https://gpui-kit.com/docs/installation/) e do
`libudev` para descoberta de portas seriais. A compilação do desktop é independente do CMake e do Pico SDK.

A partir da raiz do repositório:

```sh
cargo run --locked --manifest-path desktop/Cargo.toml
```

## Construir e instalar

Os presets do desktop usam a toolchain nativa do Rust 1.92 ou mais recente, CMake 3.24 ou mais recente e Ninja. A partir da raiz do repositório, entre
em `desktop/` antes de executar os comandos desta seção.

```sh
cd desktop
cmake --list-presets=all
```

| Sistema | Configure         | Build do instalador | Build apenas do executável |
| ------- | ----------------- | ------------------- | -------------------------- |
| Linux   | `desktop-linux`   | `installer-linux`   | `desktop-linux-release`    |
| Windows | `desktop-windows` | `installer-windows` | `desktop-windows-release`  |
| macOS   | `desktop-macos`   | `installer-macos`   | `desktop-macos-release`    |

Depois de configurar o preset do sistema, use `cmake --build --preset <preset-de-build>`. Os instaladores ficam em `build/dist/` e o executável de
release fica em `target/<triple-do-Rust>/release/`.

### Linux

No Ubuntu/Debian, instale as dependências do GPUI e do empacotamento:

```sh
sudo apt-get update
sudo apt-get install -y pkg-config clang libfontconfig-dev libwayland-dev \
  libwebkit2gtk-4.1-dev libxkbcommon-dev libxkbcommon-x11-dev libx11-xcb-dev \
  libudev-dev libssl-dev libzstd-dev libvulkan1 desktop-file-utils ninja-build
```

Em outras distribuições, instale os equivalentes de Fontconfig, Wayland, WebKitGTK, XKB/XCB, libudev, Vulkan, OpenSSL, zstd e `pkg-config`. Compile
para a arquitetura nativa:

```sh
cmake --preset desktop-linux
cmake --build --preset installer-linux
```

O resultado será `build/dist/daz-shifter-<versão>-linux-<arquitetura>.sh`. O script é autocontido e aceita `--user` (padrão) ou `--system`.

```sh
# Usuário atual: ~/.local/bin e $XDG_DATA_HOME/applications
build/dist/daz-shifter-*.sh --user

# Todos os usuários: /usr/local/bin, menu global e regra udev
sudo build/dist/daz-shifter-*.sh --system
```

Após uma instalação `--system`, desconecte e reconecte o shifter para a regra udev liberar a porta CDC de configuração. Para remover, use o mesmo
escopo:

```sh
build/dist/daz-shifter-*.sh --user --uninstall
sudo build/dist/daz-shifter-*.sh --system --uninstall
```

O instalador Linux deve ser construído na distribuição onde será executado, pois o binário usa a glibc e as bibliotecas nativas disponíveis no sistema
de build. Os artefatos do CI são compilados no Ubuntu 24.04 para x86-64 e ARM64.

### Windows

Instale o Rust MSVC, CMake, Ninja e o [Inno Setup 6.3+](https://jrsoftware.org/isinfo.php). Execute os comandos no PowerShell.

```sh
cmake --preset desktop-windows
cmake --build --preset installer-windows
```

O resultado será `build/dist/daz-shifter-<versão>-windows-x86_64.exe`. Se o Inno Setup estiver em outro diretório, informe o executável ao configurar.

```sh
cmake --preset desktop-windows '-DDAZ_ISCC=C:/Program Files (x86)/Inno Setup 6/ISCC.exe'
cmake --build --preset installer-windows
```

Abra o `.exe` para escolher o escopo na interface. **Usuário atual** instala em `%LOCALAPPDATA%\Programs\DAZ Shifter` sem elevação; **todos os
usuários** instala em `%ProgramFiles%\DAZ Shifter` e solicita elevação. Também é possível selecionar diretamente `/CURRENTUSER` ou `/ALLUSERS` em
instalações automatizadas:

```sh
$installer = '.\build\dist\daz-shifter-0.1.0-windows-x86_64.exe' # Use a versão gerada
& $installer /CURRENTUSER
# Para instalar no sistema, use /ALLUSERS em vez de /CURRENTUSER.
```

O desinstalador fica registrado nas Configurações do Windows e no diretório instalado.

### macOS

Instale as Xcode Command Line Tools (`xcode-select --install`) e use um runner nativo Intel ou Apple Silicon. O build gera um `.pkg` para a
arquitetura atual:

```sh
cmake --preset desktop-macos
cmake --build --preset installer-macos
```

O resultado será `build/dist/daz-shifter-<versão>-macos-<arquitetura>.pkg`. Abra o pacote no Finder e escolha **Apenas para mim** (aplicativo em
`~/Applications`) ou **Para todos os usuários deste computador** (aplicativo em `/Applications`). Pela linha de comando, os mesmos escopos são:

```sh
# Usuário atual
/usr/sbin/installer -pkg build/dist/daz-shifter-*.pkg -target CurrentUserHomeDirectory

# Sistema (requer senha de administrador)
sudo /usr/sbin/installer -pkg build/dist/daz-shifter-*.pkg -target /
```

Para remover, apague `DAZ Shifter.app` de `~/Applications` ou `/Applications` conforme o escopo instalado. O pacote padrão usa assinatura ad hoc para
testes locais. Para distribuição, configure certificados Developer ID antes do build:

```sh
cmake --preset desktop-macos \
  -DDAZ_MACOS_APP_SIGN_IDENTITY='Developer ID Application: Empresa (TEAMID)' \
  -DDAZ_MACOS_INSTALLER_SIGN_IDENTITY='Developer ID Installer: Empresa (TEAMID)'
cmake --build --preset installer-macos
```

A notarização é uma etapa posterior que exige uma conta Apple e credenciais próprias.

### CI e artefatos

O workflow [Desktop installers](../.github/workflows/desktop-installers.yml) constrói os cinco artefatos em runners nativos (Linux x86-64/ARM64,
Windows x64 e macOS Intel/Apple Silicon). Ele também testa instalação e remoção nos dois escopos sempre que há pull request em `desktop/**`, execução
manual ou uma tag `desktop-v*`.

### Conectar por USB CDC

Grave na placa o firmware deste repositório. O firmware de V1 e V2 oferece a porta CDC de configuração e o joystick HID. `H_SHIFTER_LOG=ON` cria uma
porta CDC separada para logs. [Instruções de gravação](../README.md#gravar-o-firmware-na-placa)

Conecte o câmbio, clique em **Buscar portas**, selecione a porta de configuração e clique em **Conectar**. No Linux, a porta costuma aparecer como
`/dev/ttyACM*` e requer acesso de leitura e escrita ao dispositivo `tty`. No Windows, aparece como porta `COM`. No macOS, aparece normalmente como
`/dev/cu.usbmodem*`. O aplicativo valida o dispositivo com `GET_INFO` antes de liberar o formulário. Use o
[gesto de desbloqueio da placa](../README.md#desbloqueio-da-configuração) para habilitar as ações. Se a gravação ficar sem confirmação, reconecte e
confira a configuração lida antes de salvar novamente.

Na V1, o formulário oferece até 6 marchas e ajustes de modo inicial e tempo de acionamento sequencial. Na V2, oferece até 7 marchas e a chave física
seleciona o modo.

## Estrutura

| Módulo                                   | Responsabilidade                                                     |
| ---------------------------------------- | -------------------------------------------------------------------- |
| `main.rs`                                | Inicializa o GPUI Kit, seleciona o tema e abre a janela.             |
| `app.rs` e `app/state.rs`                | Recebem eventos e mantêm a sessão, a configuração lida e o rascunho. |
| `ui/`                                    | Compõe os painéis com os componentes estilizados do kit.             |
| `device.rs` e `protocol.rs`              | Modelam e validam os dados do dispositivo e o formato binário.       |
| `connection.rs` e `connection/serial.rs` | Gerenciam a sessão e executam os comandos pela porta USB CDC.        |

## Verificar

A partir de `desktop/`:

```sh
cargo fmt --check
cargo test --locked
cargo clippy --all-targets --all-features --locked -- -D warnings
```

Os testes cobrem o formato binário do protocolo v1, a remontagem de quadros e a comunicação por uma porta serial virtual. Com uma V1 e uma V2, confira
descoberta de porta, diagnóstico, gravação na flash, remoção do cabo e persistência após desligar e ligar.
