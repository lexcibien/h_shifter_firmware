# TODO List

## Programação

- [X] Adicionar marcha ré V1
  - [X] Talvez colocar um switch na base e um mecanismo na manopla que quando puxa e engata a 6a marcha, ativa a marcha ré, quando volta,
esse sistema desengata V1
- [X] Fazer a lógica com o botão para habilitar/desabilitar câmbio sequencial V1
  - [X] Adicionar dois botões adicionais para o sequencial quando ativado V1
  - [X] Mudar o LED WS2812 de cor quando for H Shifter (violeta) e quando for Sequencial (laranja)
- [X] Adicionar botões da manopla do caminhão para usar ela junta
- [X] Adicionar um resistor (4k7 ou 6k8) em um dos botões em paralelo
  - Isso é para ver se a manopla está conectada (assim não fica flutuando os botões)
- [X] Desenvolver um configurador desktop em Rust para o dispositivo via USB CDC.
  - [X] Slider para a quantidade de marchas
  - [X] Configurar o modo inicial sequencial na V1
  - [X] Habilitar/desabilitar marcha ré
  - [X] Botão para checar se a manopla está conectada sem reiniciar o dispositivo
  - [X] Botão para reiniciar o dispositivo
  - [X] Usar a combinação sequencial + switch direito para liberar alterações
  - [X] Persistir configurações na flash com recuperação em caso de gravação incompleta
- [X] Alterar PlatformIO para CMake nas Actions

## Hardware

- [X] Reimprimir peças em PLA+
- [X] Colocar um conector no chicote para a manopla
- [X] Adicionar um switch para trocar de H para sequencial
- [ ] Adicionar display 14 segmentos
  - [ ] Usar um multiplexador 595 para reduzir quantidade de pinos necessários
  - [ ] Mostrar marchas
- [ ] Adicionar display redondo
  - [ ] Mostrar mapa
