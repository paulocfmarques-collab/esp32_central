# Comandos pela serial USB

Abra o Monitor Serial em **115200 baud** e selecione nova linha, retorno de
carro ou ambos. Envie um comando por linha, por exemplo:

```text
help
info
status
led_on
led_off
led_blink:1000
time
net_info
```

Todos os comandos locais existentes podem ser enviados pela serial, inclusive
configuracao e SD. Argumentos como caminhos e textos mantem a caixa original.
As respostas aparecem entre `[RESPOSTA]` e `[FIM RESPOSTA]`, alem da tela.
Os logs de inicializacao e de rede continuam na mesma porta.

O monitor tambem mostra `[ESP TX N] comando` quando a Central envia um comando
por UDP, `[ESP RX]` e `[FIM ESP RX]` envolvendo respostas dos ESPs, e
`[ESP ERRO]` quando o controle remoto detecta timeout. Esses registros sao
copias para acompanhamento; o transporte aos ESPs continua sendo UDP.

A leitura serial nao espera a linha completa e funciona sem Wi-Fi. Linhas
vazias sao ignoradas; linhas com NUL ou mais de 255 bytes sao descartadas
integralmente e geram erro ao receber CR/LF. Backspace apaga o ultimo byte.
Durante a gravacao OTA, o processamento de comandos permanece suspenso.
