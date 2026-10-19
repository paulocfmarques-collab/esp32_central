# Comandos da Central

Revisao documental: **31/10/2026**. Este guia integra o
[catalogo geral](../ESP32_Documents/COMANDOS.md) e o
[historico consolidado](../ESP32_Documents/changelog.md).

Referencia do firmware atual, baseada em [CommandHandler.cpp](CommandHandler.cpp),
[SerialCommands.cpp](SerialCommands.cpp) e [UdpComm.cpp](UdpComm.cpp).
Os comandos deste documento operam na Central, exceto quando indicados como
comandos enviados a um ESP remoto.

## Como enviar

### Serial USB

- Abra a porta USB da placa em **115200 baud**.
- Envie um comando por linha, terminado por CR, LF ou CRLF.
- O limite e **255 bytes por linha**, sem contar a terminacao.
- Linhas vazias sao ignoradas. NUL ou excesso de tamanho fazem a linha inteira
  ser descartada, com aviso ao receber a terminacao.
- Backspace remove o ultimo byte digitado.
- As respostas normais ficam entre `[RESPOSTA]` e `[FIM RESPOSTA]`.
  Logs de boot e rede compartilham a mesma serial.

O nome do comando e normalizado para minusculas e os espacos nas extremidades
sao removidos. Nos comandos SD, caminhos e textos preservam maiusculas e
minusculas. A serial funciona sem Wi-Fi; durante a gravacao OTA, seu
processamento fica suspenso. Bluetooth nao esta ativo no firmware atual.

```text
info
status
net_info
sd_status
```

### Tela e UDP

O painel **COMPUTADOR LOCAL** executa comandos na Central. Os paineis **ESP 1**
a **ESP 4** enviam comandos ao dispositivo selecionado por UDP, na porta
**4210**, com timeout de resposta de **2500 ms** no controle remoto.
Consulte [botoes.md](botoes.md) para a distribuicao das acoes nas paginas.

A Central tambem recebe comandos textuais por UDP 4210 e responde ao IP e
porta de origem. Pacotes de IPs cadastrados como ESPs sao tratados como
respostas dos dispositivos, nao como novos comandos para a Central.
Enviar `led_on` pela USB controla a Central; nao encaminha a acao ao ESP
selecionado na tela.

## Informacoes e diagnosticos

| Comando | Resultado |
|---|---|
| `name` | Identifica o dispositivo; responde `ESP32_Central`. |
| `info` | Hostname, firmware, build, rede, heap, Flash, SD, data/hora, uptime e motivo do reset. |
| `status` | Estado de Wi-Fi, SD, NTP e heap livre. |
| `version` | Versao automatica da compilacao no formato `AAMMDD.HH.MM`. |
| `build` | Data e hora da compilacao. |
| `cpu` | Modelo, revisao, nucleos e frequencia da CPU. |
| `ram` | Heap total, livre, minimo historico, maior alocacao disponivel e percentual utilizado. |
| `flash` | Tamanho, velocidade, modo, tamanho do sketch e espaco para sketch. |
| `psram` | Presenca, capacidade e memoria PSRAM disponivel. |
| `temp` | Temperatura interna do chip em graus Celsius; nao e a temperatura ambiente. |
| `uptime` | Tempo desde a inicializacao, em segundos. |
| `reason` | Motivo do ultimo reset e uptime atual. |
| `lastcmd` | Ultimo comando processado, excetuando `lastcmd` e `cmdcount`. |
| `cmdcount` | Contador de comandos desde o boot, excetuando essas duas consultas. |
| `reset_counters` | Zera somente os contadores persistentes de boot/watchdog no namespace `telemetry`; preserva as configurações Prometheus. |
| `help` | Destinado a listar comandos; veja a limitacao atual abaixo. |
| `vago` | Responde que o slot esta vazio; os botoes `[ VAGO ]` nao disparam esse comando. |

### Relatorio de crash

`crash_info` consulta o core dump interno. Havendo registro valido, salva o
relatorio no SD em `/crash_AAAAMMDD_HHMMSS.txt`, reabre e confere o conteudo
antes de apagar o registro interno da Flash.

- Sem SD gravavel ou com falha de verificacao, preserva o dump interno.
- Sem NTP, o nome usa `SEM_NTP` e uptime.
- Nomes repetidos recebem sufixo numerico.
- Nem todo crash gera um dump valido.
- O comando **nao e somente leitura**: depois de arquivar e verificar, pode
  apagar o dump interno. O motivo do reset continua disponivel em `reason`.

Guarde o ELF correspondente ao firmware para decodificar o backtrace.
Veja [SERIAL_USB.md](SERIAL_USB.md) e [SD_CARD.md](SD_CARD.md).

## Rede, relogio e configuracao

| Comando | Efeito |
|---|---|
| `mac` | Consulta o MAC Wi-Fi da Central. |
| `net_info` | Mostra SSID, IP, gateway, mascara, DNS e RSSI. |
| `scan` | Marca os quatro ESPs como offline e envia `alive` a cada IP cadastrado; as respostas atualizam a tela inicial. **Nao procura redes Wi-Fi.** |
| `time` | Consulta a hora local. |
| `date` | Consulta a data local. |
| `set_fuso:-3` | Ajusta o deslocamento UTC inteiro, de -12 a +14, sem horario de verao. |
| `set_escravo1:192.168.0.120` | Altera e persiste o IP do ESP 1. |
| `set_escravo2:192.168.0.125` | Altera e persiste o IP do ESP 2. |
| `set_escravo3:192.168.0.130` | Altera e persiste o IP do ESP 3. |
| `set_escravo4:192.168.0.135` | Altera e persiste o IP do ESP 4. |
| `reset_wifi` | Limpa o namespace `wifi` das Preferences e reinicia apos aproximadamente dois segundos. |

Os IPs dos exemplos sao os padroes quando nao ha valores salvos. Use IPv4
valido: atualmente a resposta de `set_escravoN` repete o argumento mesmo
quando o IP e invalido e a alteracao nao foi aplicada.

O fuso alterado por comando nao e persistido; o boot configura UTC-3.
`reset_wifi` remove tambem os IPs dos ESPs salvos no namespace `wifi`, nao
apaga toda a Flash, o SD ou as configuracoes `telemetry` do Prometheus.
Sem conexao salva utilizavel, o firmware abre o AP
**ESP32_CENTRAL_CONFIG**, em `192.168.4.1`.

## LED e energia

| Comando | Efeito |
|---|---|
| `led_on` | Liga o LED integrado no GPIO4, ativo em nivel baixo, e desativa o pisca. |
| `led_off` | Desliga o LED e desativa o pisca. |
| `led_blink:1000` | Alterna o LED a cada 1000 ms; um ciclo completo leva aproximadamente dois segundos. |
| `reboot` | Anuncia o reinicio na serial/tela e reinicia apos aproximadamente dois segundos. |
| `desligar` | Apaga o backlight e entra em deep sleep apos aproximadamente dois segundos. BOOT (GPIO0 em nivel baixo) acorda a Central. |

Use intervalo positivo em `led_blink`. Zero ou texto nao numerico resulta
em 500 ms; nao use valores negativos. Reinicio e deep sleep interrompem a
comunicacao; nao dependa de uma resposta UDP antes da desconexao.

## Cartao SD

Use FAT32. Caminhos devem comecar com `/` e nao podem conter `..`.
O parser usa `:` como separador; o texto de escrita pode conter outros `:`.

| Comando | Efeito |
|---|---|
| `sd_status` | Atualiza as estatisticas e mostra tipo, total e espaco utilizado. |
| `sd_remount` | Tenta montar novamente o cartao; nao o formata. |
| `sd_test` | Cria, grava, le e remove `/central_sd_test.tmp`; preserva esse arquivo se ja existir. |
| `sd_list` | Lista a raiz do cartao. |
| `sd_list:/pasta` | Lista os filhos imediatos de uma pasta. |
| `sd_read:/arquivo.txt` | Le o arquivo; se maior que 1000 bytes, retorna o trecho final com indicador `[...]`. |
| `sd_log` | Le os ultimos 1000 bytes de `/log.txt`, com o mesmo indicador se houver corte. |
| `sd_write:/arquivo.txt:Texto` | Cria ou sobrescreve o arquivo com o texto, sem acrescentar quebra de linha. |
| `sd_append:/arquivo.txt:Texto` | Acrescenta texto e quebra de linha ao arquivo. |
| `sd_del:/arquivo.txt` | Apaga um arquivo. |
| `sd_mkdir:/pasta` | Cria uma pasta. |
| `sd_rmdir:/pasta` | Remove a pasta e seu conteudo recursivamente; a raiz `/` e recusada. |
| `sd_clear_log` | Apaga `/log.txt`; o registro da resposta pode iniciar um novo log. |

Listagens tambem sao limitadas a aproximadamente 1000 caracteres.
As operacoes retornam erro se o SD estiver indisponivel.
**Escrita, exclusao, remocao de pasta e limpeza de log nao pedem confirmacao.**

Exemplo de teste que deixa um arquivo no SD:

```text
sd_status
sd_test
sd_write:/central_teste.txt:Primeira linha
sd_append:/central_teste.txt:Segunda linha
sd_read:/central_teste.txt
sd_list:/
```

Para remove-lo depois, envie `sd_del:/central_teste.txt`.
Comandos e respostas sao registrados em `/log.txt` quando o SD esta disponivel.
Detalhes de montagem, SPI e registro: [SD_CARD.md](SD_CARD.md).

## Comandos dos ESPs remotos

Os paineis remotos oferecem os seguintes comandos; o suporte efetivo depende
do firmware instalado no destino:

```text
led_on          led_off         led_blink:1000
temp            cpu             ram
uptime          heap            flash
net_info        mac             rssi
info            status          version
build           reason          help
time            date            alive
ip              reboot          reset_wifi
sd_status       sd_list         sd_log
sd_test         sd_clear_log    led_breath
```

`heap`, `rssi`, `alive`, `ip` e `led_breath` nao possuem handlers locais na
Central. Para consultar a memoria e o sinal da propria Central, use `ram`
e `net_info`.

## Limites conhecidos do firmware atual

- `help` tem sua lista sobrescrita pelo ramo de comando desconhecido no
  dispatcher atual. Este documento e a referencia enquanto isso nao for corrigido.
- O percentual de uso de `psram` nao e confiavel quando nao ha PSRAM:
  o calculo atual divide pela capacidade zero.
- Os argumentos numericos usam conversao permissiva. Envie numeros validos,
  respeitando os limites documentados.
- Comandos UDP nao possuem autenticacao; use somente uma rede confiavel.
- Prometheus e configurado pela pagina `/prometheus`, na porta HTTP 80,
  nao por comandos seriais. A coleta usa `/metrics`, por padrao na porta 9120.
  Consulte [PROMETHEUS.md](PROMETHEUS.md).

Veja tambem [SERIAL.md](SERIAL.md), [PINAGEM.md](PINAGEM.md) e
[changelog.md](changelog.md).
