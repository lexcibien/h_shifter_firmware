# Configurador DAZ Shifter

O configurador usa Rust e [GPUI Kit 0.11](https://gpui-kit.com/docs/) com o tema Default Dark. A janela reúne
as informações do dispositivo, a configuração de marcha e entradas, as cores dos LEDs e o diagnóstico em tempo real.

## Executar

Use Rust 1.92 ou mais recente e uma sessão gráfica. No Linux, instale as dependências do
[GPUI Kit](https://gpui-kit.com/docs/installation/) e do `libudev` para descoberta de portas seriais.
A compilação do desktop é independente do CMake e do Pico SDK.

A partir da raiz do repositório:

```sh
cargo run --locked --manifest-path desktop/Cargo.toml
```

## Instaladores

Gere o instalador no sistema de destino, com CMake 3.24 ou mais recente e a toolchain nativa do Rust. Execute a partir da raiz do repositório.

```sh
cmake -S desktop -B desktop/build
cmake --build desktop/build --target installer
```

O target `installer` seleciona o empacotador da plataforma e grava o artefato em `desktop/build/dist/`. A versão vem de `desktop/Cargo.toml`.
O workflow [Desktop installers](../.github/workflows/desktop-installers.yml) também gera os artefatos em runners nativos do GitHub Actions.

| Plataforma | Target | Instalação por usuário | Instalação no sistema |
| --- | --- | --- | --- |
| Linux x86-64 e ARM64 | `installer-linux` | `--user` instala em `~/.local` e cria a entrada no menu em `$XDG_DATA_HOME` | Execute com `sudo` e `--system`. Instala em `/usr/local` e cria a regra udev para a sessão local ativa. |
| Windows x64 | `installer-windows` | Escolha o usuário atual ou use `/CURRENTUSER`. Instala em `%LOCALAPPDATA%\Programs`. | Escolha todos os usuários ou use `/ALLUSERS`. O instalador solicita elevação e usa `%ProgramFiles%`. |
| macOS 15+ Intel e Apple Silicon | `installer-macos` | Escolha somente o usuário atual no Installer. O app fica em `~/Applications`. | Escolha todos os usuários. O app fica em `/Applications`. |

No Linux, execute o arquivo `.sh` gerado com o escopo escolhido. Guarde o arquivo para desinstalar usando o mesmo escopo com `--uninstall`.
O binário usa a glibc e as bibliotecas nativas da distribuição de build. Os artefatos do CI são compilados no Ubuntu 24.04.
Para executar, tenha uma sessão gráfica, um driver Vulkan e as bibliotecas de runtime de Fontconfig, XCB, XKB e libudev.
Na instalação por usuário, o acesso à porta serial depende das permissões da distribuição. Na instalação no sistema, reconecte o shifter após instalar a regra udev.

No Windows, use a toolchain MSVC e o **Inno Setup 6.3 ou mais recente**. Se necessário, indique o compilador no primeiro comando com
`-DDAZ_ISCC="C:/caminho/ISCC.exe"`. O instalador cria um atalho no menu Iniciar e uma entrada de desinstalação nas Configurações do Windows.

No macOS, instale as Xcode Command Line Tools. Use o Instalador do macOS para abrir o `.pkg` e o Finder para remover o app da pasta escolhida.
O build padrão gera um pacote para testes locais. Para assinar, configure `DAZ_MACOS_APP_SIGN_IDENTITY` e `DAZ_MACOS_INSTALLER_SIGN_IDENTITY`
com os certificados Developer ID correspondentes. A notarização é uma etapa de distribuição feita após o empacotamento.
[Empacotamento para distribuição no macOS](https://developer.apple.com/documentation/xcode/packaging-mac-software-for-distribution)

### Conectar por USB CDC

Grave na placa o firmware deste repositório. O firmware de V1 e V2 oferece a porta CDC de configuração e o joystick HID.
`H_SHIFTER_LOG=ON` cria uma porta CDC separada para logs. [Instruções de gravação](../README.md#gravar-o-firmware-na-placa)

Conecte o câmbio, clique em **Buscar portas**, selecione a porta de configuração e clique em **Conectar**.
No Linux, a porta costuma aparecer como `/dev/ttyACM*` e requer acesso de leitura e escrita ao dispositivo `tty`.
No Windows, aparece como porta `COM`. No macOS, aparece normalmente como `/dev/cu.usbmodem*`.
O aplicativo valida o dispositivo com `GET_INFO` antes de liberar o formulário.
Use o [gesto de desbloqueio da placa](../README.md#desbloqueio-da-configuração) para habilitar as ações.
Se a gravação ficar sem confirmação, reconecte e confira a configuração lida antes de salvar novamente.

Na V1, o formulário oferece até 6 marchas e ajustes de modo inicial e tempo de acionamento sequencial.
Na V2, oferece até 7 marchas e a chave física seleciona o modo.

## Estrutura

| Módulo | Responsabilidade |
| --- | --- |
| `main.rs` | Inicializa o GPUI Kit, seleciona o tema e abre a janela. |
| `app.rs` e `app/state.rs` | Recebem eventos e mantêm a sessão, a configuração lida e o rascunho. |
| `ui/` | Compõe os painéis com os componentes estilizados do kit. |
| `device.rs` e `protocol.rs` | Modelam e validam os dados do dispositivo e o formato binário. |
| `connection.rs` e `connection/serial.rs` | Gerenciam a sessão e executam os comandos pela porta USB CDC. |

## Verificar

A partir de `desktop/`:

```sh
cargo fmt --check
cargo test --locked
cargo clippy --all-targets --all-features --locked -- -D warnings
```

Os testes cobrem o formato binário do protocolo v1, a remontagem de quadros e a comunicação por uma porta serial virtual.
Com uma V1 e uma V2, confira descoberta de porta, diagnóstico, gravação na flash, remoção do cabo e persistência após desligar e ligar.
