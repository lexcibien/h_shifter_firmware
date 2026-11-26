# Configurador DAZ Shifter em GPUI

Esta janela usa Rust e [GPUI 0.2.2](https://docs.rs/gpui/0.2.2/gpui/). Ela permite simular a conexão de um DAZ Shifter V1 e alterar localmente a
quantidade de marchas entre 4 e 6. O exemplo roda sem a placa. Os valores voltam ao padrão ao fechar o programa.

## Rodar e experimentar

Use uma versão estável recente do Rust com Cargo e uma sessão gráfica. No Linux, o GPUI habilita Wayland e X11 por padrão. Use um driver Vulkan para a
GPU e tenha disponíveis `pkg-config` e as bibliotecas de desenvolvimento Fontconfig, `libxkbcommon` e `libxkbcommon-x11`. A compilação é independente
do CMake e do Pico SDK. [Recursos do GPUI 0.2.2](https://docs.rs/crate/gpui/0.2.2/features) e
[requisitos do backend Vulkan](https://docs.rs/crate/blade-graphics/0.7.1/source/README.md).

A partir da raiz do repositório, execute `cargo run --locked --manifest-path desktop/Cargo.toml`. Clique em **Simular conexão**, altere o número de
marchas com **−** e **+** e observe a janela atualizar. **Desconectar** alterna o estado da sessão simulada. O ajuste de marchas é um rascunho local.

O `Cargo.lock` registra versões de `libc` e `mio` compatíveis com a dependência `xattr` usada pelo GPUI 0.2.2. Ao atualizar o lockfile, confira a
compilação no Linux. [Problema registrado no repositório do GPUI](https://github.com/zed-industries/zed/issues/67211).

| Arquivo                                      | Onde praticar                                                                                                                                                                                    |
| -------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| [`src/main.rs`](src/main.rs)                 | `Application::run` inicia o GPUI. `open_window` define a janela, e `cx.new` cria a entidade raiz. Experimente alterar o título e o tamanho inicial.                                              |
| [`src/configurator.rs`](src/configurator.rs) | `Configurator` guarda o estado, `Render` monta os elementos com `div()` e `cx.listener` liga os cliques ao estado. Depois de alterar o estado, `cx.notify()` pede que a janela seja redesenhada. |

## Roteiro de migração

Hoje os jogos recebem os botões pela interface **HID**. O navegador configura o câmbio por **WebUSB**, numa interface USB separada. O **USB CDC**
(porta serial virtual) só é habilitado no build com `H_SHIFTER_LOG=ON`, que o usa para logs de texto. Para o configurador conversar por serial, o
firmware também precisa aceitar os comandos pela CDC. Você pode manter WebUSB e CDC durante a transição e continuar usando o HID para os jogos.
[Descritores atuais](../src/usb/usb_descriptors.c) e [opções do firmware](../src/CMakeLists.txt).

### 1. Aprenda GPUI com dados simulados

1. Acrescente um indicador de modo **H / Sequencial** com um valor de exemplo. Para praticar eventos, adicione um controle de **Marcha ré** seguindo o
   padrão do ajuste de marchas.
2. Modele conexão, informações do dispositivo, configuração e diagnóstico em tipos Rust. Comece com dados simulados para V1 e V2. Use `maxGears` para
   o limite de marchas e os bits de `capabilities` para mostrar **Modo inicial** e **Tempo de acionamento sequencial**. A V2 usa a chave física para o
   modo e comporta até 7 marchas à frente.
3. Adicione estados de carregamento, dispositivo ausente e falha de comunicação. Experimente navegação com Tab e ativação por teclado ao evoluir os
   controles. [Context do GPUI](https://docs.rs/gpui/0.2.2/gpui/struct.Context.html) e
   [exemplo de navegação por Tab na versão 0.2.2](https://docs.rs/crate/gpui/0.2.2/source/examples/tab_stop.rs).

**Concluído quando** a interface alternar entre exemplos de V1 e V2 e refletir as capacidades de cada versão sem precisar da placa.

### 2. Porte o protocolo para Rust

Crie `desktop/src/protocol.rs` para codificar os pedidos e interpretar as respostas, com tipos que possam ser testados sem GPUI ou hardware. Use
[`web/protocol.js`](../web/protocol.js) e [`WebUsbProtocol.cpp`](../src/usb/WebUsbProtocol.cpp) como referência.

- Preserve o quadro de 64 bytes, o cabeçalho `HS`, a versão `1`, o comando, a transação de 16 bits em little-endian e o tamanho de payload de até 52
  bytes. O CRC-32 IEEE cobre os primeiros 60 bytes. Na resposta, o comando tem o bit `0x80` ligado e o primeiro byte do payload informa o status.
- Implemente primeiro `GET_INFO` (8 bytes de dados) e `GET_STATUS` (7 bytes). Depois implemente `GET_CONFIG` (20 bytes de configuração, 4 de geração e
  1 de origem). Valide tamanhos antes de ler campos e confira versão, CRC, comando de resposta e transação.
- Reaproveite os casos de [`web/protocol.test.js`](../web/protocol.test.js) e compare os bytes produzidos pelo Rust aos do JavaScript para os mesmos
  pedidos. Rode `npm --prefix web test` para ter a referência web e `cargo test --manifest-path desktop/Cargo.toml` ao adicionar os testes Rust.

**Concluído quando** o Rust interpretar quadros do protocolo v1, produzir os mesmos bytes do web e rejeitar quadros truncados, corrompidos ou de outra
transação.

### 3. Habilite comandos pela CDC e valide no terminal

1. No firmware, dê à serial de configuração uma opção de build própria. Uma primeira implementação pode usar a CDC existente para quadros binários com
   o `stdio` USB desligado. Para usar configuração e logs ao mesmo tempo, encaminhe os logs para outra saída, como UART ou outra interface CDC com
   endpoints próprios. [CDC no TinyUSB](https://docs.tinyusb.org/en/latest/reference/class/cdc.html).
2. Extraia a validação e montagem de quadros de `WebUsbProtocol` para reaproveitá-las nos adaptadores WebUSB e CDC. Reutilize o
   [tratamento de comandos de `ShifterApp`](../src/app/ShifterApp.cpp) para que ambos passem pelo mesmo desbloqueio físico e pela mesma lógica de
   flash. Integre a CDC ao loop junto ao `tud_task()` com leitura e escrita não bloqueantes. Preserve os descritores HID e WebUSB e confira os números
   das interfaces, inclusive as referências nos descritores Microsoft OS, se o layout mudar. [Descritores atuais](../src/usb/usb_descriptors.c).
3. Na CDC, trate a entrada como fluxo de bytes. Uma leitura pode trazer parte de um quadro ou vários quadros. Acumule bytes, procure `HS` e confirme
   quadro completo pelo CRC e pelo tamanho. Em caso de perda de alinhamento, avance até um novo candidato e descarte o estado parcial ao encerrar a
   sessão. O tratamento do pacote USB de comprimento zero do web fica no adaptador WebUSB.
4. No PC, faça um programa de terminal pequeno que liste portas com `serialport::available_ports()`, abra a selecionada com timeout e execute
   `GET_INFO`. Use VID/PID `2e8a:0003` e o número de série, quando disponível, para ajudar a selecionar. Confirme que a porta fala com o câmbio usando
   `GET_INFO`. No Linux costuma aparecer como `/dev/ttyACM*`, com permissões próprias no dispositivo `tty`. A
   [regra atual](../assets/99-daz-shifter.rules) atende a interface USB usada pelo web. No Windows, procure uma porta `COM`.
   [Biblioteca serialport](https://docs.rs/serialport/latest/serialport/).

**Concluído quando** o programa de terminal imprimir projeto e versão reais pela CDC, e o joystick HID e o configurador web continuarem funcionando
com o mesmo firmware.

### 4. Ligue a serial à janela

- Substitua **Simular conexão** pela seleção de porta e mantenha o modo simulado para praticar sem hardware. Abra a porta numa thread dedicada, já que
  `serialport` usa I/O bloqueante. Deixe essa thread ser dona da porta e receber os comandos por um canal. Na tarefa da UI, receba os resultados por
  um canal assíncrono, atualize a entidade GPUI e chame `cx.notify()`.
- Comece por `GET_INFO`, `GET_CONFIG` e `GET_STATUS`. Reproduza o diagnóstico do web usando modo, manopla, ADC, máscara de entradas e máscara HID. Use
  o intervalo de 300 ms do web como ponto de partida para `GET_STATUS`, com um pedido por vez para evitar que a consulta acumule na fila.
- Ao desconectar, pare as consultas, feche a porta, limpe o estado do dispositivo e ignore respostas da sessão anterior. Diferencie porta ausente,
  permissão negada, timeout e resposta inválida na interface. [Fluxo atual no web](../web/app.js).

**Concluído quando** for possível conectar, ver o estado mudar ao mexer no câmbio, retirar o cabo e reconectar com a janela aberta e responsiva.

### 5. Migre a edição e a gravação

- Mostre a origem e a geração de `GET_CONFIG`. Guarde separadamente a última configuração lida e o rascunho editável. Use os limites de
  [`DeviceConfig.cpp`](../src/config/DeviceConfig.cpp) para validar valores, incluindo debounce de 5–50 ms, acionamento sequencial de 250–3000 ms e
  limiar da manopla de 500–4000.
- Use a flag de desbloqueio em `GET_STATUS` para habilitar **Salvar**, **Restaurar padrões**, **Redetectar manopla** e **Reiniciar**. Mostre a
  instrução da V1 ou da V2 e trate `LOCKED` devolvido pelo firmware. Peça confirmação para restaurar padrões e reiniciar.
  [Gestos de desbloqueio](../README.md#configurador-webusb).
- Depois de `SAVE_CONFIG`, leia `GET_CONFIG` para conferir conteúdo, origem na flash e geração antes de indicar sucesso, como o web faz. Se a resposta
  se perder, leia novamente a configuração ao reconectar antes de tentar outra gravação. Após reiniciar, trate a porta como desconectada até ela
  reaparecer.

**Concluído quando** a configuração lida após desligar e ligar a placa for igual à configuração confirmada pela janela.

### 6. Valide antes de distribuir

Compile os presets V1 e V2 e rode os [testes do firmware](../README.md#executar-os-testes). Com as duas placas, verifique no desktop e no web se o
diagnóstico funciona com a configuração bloqueada. Teste o gesto de desbloqueio, a gravação na flash, a restauração, a perda de conexão durante um
pedido e o funcionamento do joystick. Teste descoberta de porta e permissões em cada sistema operacional em que pretende distribuir. Gere a primeira
versão de teste com `cargo build --release --locked --manifest-path desktop/Cargo.toml`.
