# Pinagem da ESP32-2432S028

Este documento resume os pinos identificados no diagrama da placa e distingue
a pinagem usada pelo firmware da rotulagem impressa no modulo. A imagem de
referencia e a [manual-pinagem.png](manual-pinagem.png), da ESP32-2432S028
Revision 3.

## Perifericos usados pelo firmware

| Periferico | Sinal | GPIO | Observacao |
|---|---|---:|---|
| TFT ILI9341 | MISO / SDO | 12 | Leitura SPI do display. |
| TFT ILI9341 | MOSI / SDI | 13 | Escrita SPI do display. |
| TFT ILI9341 | SCLK | 14 | Clock SPI do display. |
| TFT ILI9341 | CS | 15 | Selecao do display. |
| TFT ILI9341 | DC / RS | 2 | Selecao de comando/dados. |
| TFT ILI9341 | Backlight | 21 | Ativo em nivel alto. |
| TFT ILI9341 | Reset | — | Sem GPIO dedicado (`TFT_RST=-1`). |
| Touch XPT2046 | CLK | 25 | SPI do touch (HSPI). |
| Touch XPT2046 | MISO / T_OUT | 39 | Leitura SPI do touch. |
| Touch XPT2046 | MOSI / T_DIN | 32 | Escrita SPI do touch. |
| Touch XPT2046 | CS / T_CS | 33 | Selecao do touch. |
| Touch XPT2046 | IRQ / T_IRQ | 36 | Identificado no diagrama; o firmware atual consulta o touch por polling. |
| Cartao TF/SD | CS | 5 | Selecao do cartao. |
| Cartao TF/SD | CLK | 18 | Clock SPI do cartao. |
| Cartao TF/SD | MOSI / CMD | 23 | Entrada de dados do cartao. |
| Cartao TF/SD | MISO / DAT0 | 19 | Saida de dados do cartao. |

Os pinos do TFT estao definidos em `build_opt.h`; os do touch, em
`Botao.h`; os do SD, em `SdManager.h`. O firmware usa o HSPI para o touch.
O acesso ao SD reutiliza temporariamente o SPI do TFT sob mutex e restaura a
configuracao do display ao terminar. Nao conecte outro dispositivo a esses
pinos sem verificar conflitos com os perifericos integrados.

Para o cartao, siga os sinais indicados junto ao conector TF: CMD = GPIO23,
CLK = GPIO18 e DAT0 = GPIO19. Algumas legendas MISO/MOSI sobre o modulo ESP
no diagrama estao invertidas em relacao aos sinais do conector TF e a
configuracao usada pelo firmware.

## Outros sinais e conectores do diagrama

| Componente/conector | Pinos indicados |
|---|---|
| Botao BOOT | GPIO0 |
| LED integrado | GPIO4; o firmware acende em nivel baixo. |
| Header P1 (serial/alimentacao) | VIN, TX, RX, GND |
| Header P3 (IO estendido) | GND, GPIO35, GPIO22, GPIO21 |
| Header CN1 (temperatura/umidade) | GND, NC, GPIO27, 3.3 V |
| Header P4 (alto-falante) | VO2, VO1, GND |
| USB1 | Micro-USB; o conversor CH340C fornece a interface serial USB. |

Confira tensoes, orientacao e revisao da placa antes de ligar acessorios.
Os nomes de pinos de um conector nao garantem compatibilidade eletrica com
qualquer modulo externo.
