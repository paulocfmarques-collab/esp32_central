# Comunicacao serial USB

## Prometheus e diagnosticos

O Wi-Fi tambem oferece `/metrics` na porta configurada e persistida (padrao
**9120**). Painel e formulario `/prometheus` permanecem na **porta 80**,
em STA e no AP. Endereco do servidor, porta de metricas e intervalo sugerido
sao salvos em Preferences `telemetry`. O modelo e coleta HTTP pelo servidor,
nao envio ativo; o YAML baixado precisa ser aplicado no Prometheus.

Metricas incluem CPU, memoria, rede, HTTP, temperatura interna, idade do
cache de clima e SD (capacidade, ocupacao, montagem e log). Energia, bateria,
sensores locais, reles, ADC e MQTT nao possuem integracao e nao geram leituras
ficticias. USB e UDP continuam funcionando como antes; nao ha novos comandos.
O catalogo, as regras de portas e a verificacao estao em
[PROMETHEUS.md](PROMETHEUS.md).

Bluetooth foi removido do firmware. Selecione a porta USB da placa no
testador ou Monitor Serial da Arduino IDE, usando 115200 baud e finalizacao
CR, LF ou CRLF. Envie `help` para consultar os comandos. No testador, escolha
o transporte Serial/USB; a porta Bluetooth COM12 nao se aplica a esta versao.
O conector Micro-USB usa a interface serial do CH340C; veja o mapa de
conectores e a pinagem da placa em [PINAGEM.md](PINAGEM.md).

O Wi-Fi continua atendendo a pagina web, OTA, clima e comunicacao com os ESPs.
Os comandos e respostas da serial continuam registrados em `/log.txt`.

`crash_info` le o registro interno e salva o relatorio no SD em um arquivo
separado `/crash_AAAAMMDD_HHMMSS.txt`. Confere o conteudo reabrindo o arquivo
antes de apagar o registro interno. Sem SD gravavel, preserva o registro.
Sem hora sincronizada, usa `SEM_NTP` e uptime no nome. Nomes repetidos recebem
um sufixo numerico. O comando `reason` continua indicando o motivo do ultimo
reset mesmo depois de apagar o core dump.

Para falhas de Prometheus sem reset, use `metrics_diag` e `ram`. O primeiro
mostra a ultima tentativa de exportacao, memoria utilizavel, estado do Matrix
e fila de comandos; o segundo inclui RAM livre e maior bloco de 8 bits.
