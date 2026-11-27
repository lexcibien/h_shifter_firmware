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

### Conectar por USB CDC

Grave na placa o firmware deste repositório. O firmware de V1 e V2 oferece a porta CDC de configuração e o joystick HID.
`H_SHIFTER_LOG=ON` cria uma porta CDC separada para logs. [Instruções de gravação](../README.md#gravar-o-firmware-na-placa)

Conecte o câmbio, clique em **Buscar portas**, selecione a porta de configuração e clique em **Conectar**.
No Linux, a porta costuma aparecer como `/dev/ttyACM*` e requer acesso de leitura e escrita ao dispositivo `tty`.
No Windows, aparece como porta `COM`. O aplicativo valida o dispositivo com `GET_INFO` antes de liberar o formulário.
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
