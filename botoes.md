# Botoes da Central

Este guia descreve os controles da tela TFT da Central ESP32-2432S028. Os
botoes de comando sao areas sensiveis ao toque; nao sao botoes fisicos da placa.

## Tela inicial

- **COMPUTADOR LOCAL** abre o painel de comandos da propria Central.
- **ESP 1**, **ESP 2**, **ESP 3** e **ESP 4** abrem o painel remoto do ESP
  correspondente. Cada botao mostra o IP configurado para aquele destino.
  Verde indica que o dispositivo foi reconhecido como online; vermelho indica
  que ainda nao foi reconhecido como online.
- **Diagnosticos** abre as paginas de informacoes da Central.

## Navegacao dos paineis

Os paineis local e remoto tem cinco paginas, com seis botoes de acao por pagina.
Toque nos botoes **< ANTERIOR** e **PROXIMA >**, na faixa inferior, para mudar
de pagina. A navegacao circula: depois da pagina 5 vem a pagina 1, e antes da
pagina 1 vem a pagina 5. **MENU** retorna a tela inicial.

Em um painel de comandos, tocar em uma acao executa o comando correspondente.
No painel local, ele e processado pela Central; no remoto, e enviado por UDP
ao ESP selecionado. A resposta pode ser mostrada na tela. Toque na area do
texto para fechar a resposta; se houver mais texto, use as setas na lateral
direita para rolar. A resposta fecha automaticamente apos 30 segundos.

### Computador local — pagina 1

| Botao | Acao |
|---|---|
| `TIME` | Consulta a hora. |
| `DATE` | Consulta a data. |
| `MAC` | Consulta o endereco MAC. |
| `TEMP` | Consulta a temperatura interna. |
| `CPU` | Consulta informacoes da CPU. |
| `RAM` | Consulta a memoria RAM. |

### Computador local — pagina 2

| Botao | Acao |
|---|---|
| `FLASH` | Consulta informacoes da memoria Flash. |
| `SCAN` | Envia `alive` aos quatro ESPs por UDP para atualizar seus estados na tela inicial; nao procura redes Wi-Fi. |
| `UPTIME` | Consulta o tempo ligado. |
| `MAC` | Consulta o endereco MAC. |
| `NET` | Consulta informacoes da rede. |
| `RST WIFI` | Apaga o namespace `wifi`, incluindo redes e IPs salvos dos ESPs, e reinicia a Central; sera necessario reconfigurar. |

### Computador local — pagina 3

| Botao | Acao |
|---|---|
| `INFO` | Exibe informacoes gerais do dispositivo. |
| `VERSION` | Exibe a versao do firmware. |
| `BUILD` | Exibe a data e hora da compilacao. |
| `STATUS` | Exibe o estado geral da Central. |
| `LAST CMD` | Exibe o ultimo comando processado localmente. |
| `CMD COUNT` | Exibe a quantidade de comandos processados localmente. |

### Computador local — pagina 4

| Botao | Acao |
|---|---|
| `REASON` | Exibe o motivo do ultimo reset. |
| `HELP` | Envia `help`; no dispatcher atual a lista e sobrescrita por comando desconhecido. Consulte [comandos.md](comandos.md). |
| `PSRAM` | Consulta informacoes de PSRAM. |
| `DESLIGAR` | Coloca a Central em deep sleep. Pressione o botao fisico BOOT (GPIO0) para acorda-la. |
| `REBOOT` | Reinicia a Central. |
| `[ VAGO ]` | Reservado; nao executa uma acao. |

### Computador local — pagina 5

| Botao | Acao |
|---|---|
| `SD STATUS` | Consulta o estado e o espaco do cartao SD. |
| `SD LISTA` | Lista os arquivos e diretorios do cartao SD. |
| `SD LOG` | Le o arquivo `/log.txt`. |
| `SD TESTE` | Testa gravacao, leitura e remocao de um arquivo temporario no SD. |
| `LIMPA LOG` | Apaga `/log.txt` do cartao SD. |
| `[ VAGO ]` | Reservado; nao executa uma acao. |

> **Atencao:** `RST WIFI` apaga a configuracao de Wi-Fi; `DESLIGAR` suspende a
> Central ate o acionamento de BOOT; `REBOOT` reinicia o dispositivo; e
> `LIMPA LOG` remove o arquivo de log do SD.

## Controle remoto

Os botoes remotos enviam comandos ao ESP selecionado na tela inicial. Os
comandos abaixo aparecem nas mesmas paginas para os ESP 1 a 4.

### Pagina 1

| Botao | Acao |
|---|---|
| `LED ON` | Liga o LED do ESP remoto. |
| `LED OFF` | Desliga o LED do ESP remoto. |
| `BLINK 1s` | Envia `led_blink:1000`; o significado do parametro depende do firmware remoto (intervalo em alguns projetos, duracao no P4). |
| `TEMP` | Consulta a temperatura. |
| `CPU` | Consulta informacoes da CPU. |
| `RAM` | Consulta a memoria RAM. |

### Pagina 2

| Botao | Acao |
|---|---|
| `UPTIME` | Consulta o tempo ligado. |
| `HEAP` | Consulta a memoria heap. |
| `FLASH` | Consulta informacoes da memoria Flash. |
| `NET` | Consulta informacoes da rede. |
| `MAC` | Consulta o endereco MAC. |
| `RSSI` | Consulta a intensidade do sinal Wi-Fi. |

### Pagina 3

| Botao | Acao |
|---|---|
| `INFO` | Exibe informacoes gerais do ESP remoto. |
| `STATUS` | Consulta o estado do ESP remoto. |
| `VERSION` | Exibe a versao do firmware. |
| `BUILD` | Exibe a data e hora da compilacao. |
| `REASON` | Exibe o motivo do ultimo reset. |
| `HELP` | Exibe os comandos aceitos pelo ESP remoto. |

### Pagina 4

| Botao | Acao |
|---|---|
| `TIME` | Consulta a hora. |
| `DATE` | Consulta a data. |
| `ALIVE` | Verifica se o ESP remoto esta respondendo. |
| `IP` | Consulta o endereco IP. |
| `REBOOT` | Reinicia o ESP remoto. |
| `RST WIFI` | Apaga as credenciais Wi-Fi do ESP remoto e reinicia-o. |

### Pagina 5

| Botao | Acao |
|---|---|
| `SD STATUS` | Consulta o estado e o espaco do cartao SD remoto. |
| `SD LISTA` | Lista os arquivos e diretorios do SD remoto. |
| `SD LOG` | Le o arquivo `/log.txt` remoto. |
| `SD TESTE` | Testa gravacao, leitura e remocao de um arquivo temporario no SD remoto. |
| `LIMPA LOG` | Apaga `/log.txt` do SD remoto. |
| `BREATH` | Ativa o efeito de respiracao do LED remoto. |

Os comandos de cartao SD e de reinicio so funcionam se o firmware do ESP
remoto oferecer essas operacoes. `RST WIFI` apaga a configuracao de Wi-Fi e
causa uma reinicializacao do dispositivo.

## Diagnosticos

A tela de diagnosticos tem seis paginas, atualizadas periodicamente:

1. **DISPOSITIVO** — modelo, revisao, CPU, memoria e build.
2. **TEMPO / RELOGIO** — sincronizacao NTP, data, hora e tempo ligado.
3. **REDE WI-FI** — estado, SSID, IP, gateway, mascara, DNS e MAC.
4. **SINAL DA REDE** — RSSI atual e historico do sinal Wi-Fi.
5. **CLIMA / PORTO ALEGRE** — leitura e previsao do clima, quando disponiveis.
6. **CARTAO SD** — estado, capacidade, uso e estatisticas do log.

Na faixa inferior, **< ANTERIOR** e **PROXIMA >** percorrem as seis paginas em
ciclo; **MENU** volta a tela inicial. Se nao houver toque por 30 segundos, a
Central retorna automaticamente ao menu.

## Tela de protecao

Depois de cinco minutos sem atividade, a Central ativa uma animacao de
protecao. Toque na tela para reabrir o menu inicial.

## Botao fisico BOOT

O botao **BOOT** da placa (GPIO0) nao navega nos menus. Alem de ser o botao de
boot da placa, ele acorda a Central quando ela esta em deep sleep.
