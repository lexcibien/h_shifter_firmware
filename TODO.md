# TODO List

## Programação

- [ ] Adicionar marcha ré
  - [ ] Talvez colocar um switch na base e um mecanismo na manopla que quando puxa e engata a 6a marcha, ativa a marcha ré, quando volta,
esse sistema desengata
- [ ] Fazer a lógica com o switch para habilitar/desabilitar câmbio sequencial
  - [ ] Adicionar dois botões adicionais para o sequencial quando ativado (ver se dá de fazer condicional ou permanente, se ele perde
a configuração no jogo)
- [X] Adicionar botões da manopla do caminhão para usar ela junta

## Hardware

- [ ] Adicionar um resistor (4k7 ou 6k8) em um dos botões em paralelo
  - Isso é para ver se a manopla está conectada (assim não fica flutuando os botões)
- [X] Reimprimir peças em PLA+
- [X] Colocar um conector no chicote para a manopla
- [ ] Adicionar um switch para trocar de H para sequencial
- [ ] Adicionar display 14 segmentos
  - [ ] Usar um multiplexador 595 para reduzir quantidade de pinos necessários
  - [ ] Mostrar marchas
- [ ] Adicionar display redondo
  - [ ] Mostrar mapa ou direções
  - [ ] Usar biblioteca TFT_eSPI ou squareline studio LVGL
