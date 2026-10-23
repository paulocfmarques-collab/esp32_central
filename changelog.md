# Changelog da Central

Resumo das mudancas do projeto Central. As entradas datadas abaixo foram
reconstruidas a partir dos commits indicados; nao representam releases
publicadas nem um historico exaustivo de todos os backups.

A versao exibida pelo firmware e gerada na compilacao no formato
`AAMMDD.HH.MM`. Consulte `version` e `build` para identificar o firmware
instalado; a data de um commit nao determina a versao gravada na placa.

## Nao publicado

### Formato do comando help

- Padronizada a resposta com o cabeçalho `COMANDOS` e um comando por linha,
  removendo texto explicativo da lista para facilitar o parseamento.
- Atualizados os guias local e consolidado.

### Contadores persistentes

- Adicionado `reset_counters` para zerar somente o registro de boot/watchdog
  no namespace `telemetry`, preservando as preferencias Prometheus.
- Atualizados o menu de comandos e os guias local e consolidado.

### Comandos

- Documentado `name`, que retorna `ESP32_Central` para identificação da placa.

### Documentacao

- Sincronizado o catalogo geral de todas as placas, com referencias aos
  [comandos locais](comandos.md) e ao
  [historico ESP32](../ESP32_Documents/changelog.md).

- Adicionado [comandos.md](comandos.md), com comandos locais e remotos,
  transportes, exemplos, efeitos destrutivos e limites conhecidos.
- Adicionado [botoes.md](botoes.md), com os paineis local/remoto, diagnosticos,
  navegacao, protetor de tela e uso do botao BOOT.
- Adicionado este changelog.
- Corrigida a descricao do botao SCAN: ele consulta os ESPs por UDP, nao
  realiza busca de redes Wi-Fi.

Essas alteracoes sao documentais; nao modificam o firmware.

## 2026-10-18

Referencia Git: `5ee4231`.

### Adicionado

- Handler local de `reboot`, com aviso na serial/tela e reinicio apos
  aproximadamente dois segundos.
- [PINAGEM.md](PINAGEM.md), distinguindo os sinais usados pelo firmware das
  legendas do diagrama da ESP32-2432S028 Revision 3.

### Alterado

- Reorganizado o mapeamento dos botoes de comandos locais e remotos.
- Atualizadas as referencias de pinagem do cartao TF/SD e da serial USB.
- Documentada a configuracao de compilacao sem PSRAM.

## 2026-10-16

Referencia Git: `32f788d`.

### Adicionado

- Servico de metricas Prometheus e [PROMETHEUS.md](PROMETHEUS.md).
- Configuracao web de endereco de referencia do servidor, porta de metricas
  e intervalo sugerido, persistida no namespace `telemetry`.
- Download de YAML para aplicar no servidor Prometheus.
- Metricas de sistema, rede, CPU, memoria, HTTP, temperatura interna,
  idade do clima e cartao SD.

### Alterado

- Painel/configuracao HTTP mantidos na porta 80; coleta `/metrics` na porta
  configurada, por padrao 9120.
- Estatisticas do SD exportadas por cache, sem novas operacoes SPI por coleta.
- Documentacao serial e SD atualizada para explicar o modelo de coleta HTTP
  pelo servidor, sem envio ativo da Central.

## 2026-10-15

Referencia Git: `9121dcb`.

### Adicionado

- Diagnostico de crash separado em
  [CrashDiagnostics.cpp](CrashDiagnostics.cpp).
- Arquivamento de `crash_info` no SD, com verificacao da copia antes de apagar
  o core dump interno. Falhas preservam o registro interno.
- [SERIAL_USB.md](SERIAL_USB.md), descrevendo a comunicacao atual pela USB.

### Removido

- Integracao Bluetooth do firmware e seus arquivos de comandos.
  [BLUETOOTH.md](BLUETOOTH.md) permanece como registro historico, nao como
  instrucao de conexao para a versao atual.

### Alterado

- Logs de inicializacao e documentacao ajustados para serial USB.
- Mantidos Wi-Fi, pagina web, OTA, clima e comunicacao UDP com os ESPs.

## 2026-10-14

Referencias Git: `e9c67af` e `2ee6123`.

### Alterado

- Revisadas inicializacao e recuperacao do Bluetooth na implementacao
  historica, com ajustes de consumo de memoria e coexistencia com Wi-Fi.
- Atualizados renderizacao de diagnosticos, acesso ao SD e consulta de clima.
- Acrescentados suporte HTTP ao clima e verificacao de fontes do display.

O Bluetooth dessa etapa foi removido no commit de 2026-10-15.

## 2026-10-13

Referencia Git: `ff1d104`.

### Adicionado

- Integracao historica de comandos por Bluetooth e sua documentacao.
- Versoes PDF das documentacoes de SD e serial.

Essa integracao nao esta ativa no firmware atual.

## 2026-10-12

Referencia Git: `2dcbb82`.

### Adicionado

- Aba web **Log do SD**, com leitura das linhas completas mais recentes de
  `/log.txt`, limitada a 16 KB, ao abrir a aba ou solicitar atualizacao.

### Alterado

- Protetor de tela Matrix apos cinco minutos sem toque, mantendo o display
  ligado; um toque retorna ao menu.
- Documentado o retorno dos diagnosticos ao menu apos 30 segundos sem toque.

## Referencias atuais

- [Comandos](comandos.md)
- [Botoes e navegacao](botoes.md)
- [Serial USB](SERIAL_USB.md)
- [Cartao SD e logs](SD_CARD.md)
- [Prometheus](PROMETHEUS.md)
- [Pinagem](PINAGEM.md)
