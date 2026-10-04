## Comandos suportados

### Comandos locais implementados no código

| Comando | Descrição |
| --- | --- |
| `help` | lista os comandos e a sintaxe aceita |
| `info` | mostra status completo do dispositivo central |
| `reason` | informa o motivo do último reset do ESP32 |
| `vago` | placeholder para comando vazio / não utilizado |
| `version` | exibe a versão do firmware do mestre |
| `build` | mostra a data e hora de compilação |
| `status` | resumo rápido de rede, NTP e heap |
| `reset` | reinicia o ESP32 |
| `time` | retorna somente a hora atual do relógio |
| `date` | retorna a data atual do relógio |
| `lastcmd` | mostra o último comando processado |
| `cmdcount` | mostra a contagem total de comandos processados |
| `set_escravo1:IP` | configura o IP do escravo 1 |
| `set_escravo2:IP` | configura o IP do escravo 2 |
| `set_escravo3:IP` | configura o IP do escravo 3 |
| `set_escravo4:IP` | configura o IP do escravo 4 |
| `reset_wifi` | limpa a flash e reinicia em modo de configuração |
| `desligar` | desliga o display e coloca o módulo em deep sleep |
| `led_on` | liga o LED de status |
| `led_off` | desliga o LED de status |
| `led_blink:500` | ativa o LED em modo de pisca-pisca com intervalo em ms |
| `set_fuso:-3` | altera o fuso horário do NTP |
| `cpu` | informa modelo do chip, núcleos e frequência |
| `ram` | mostra heap total e livre |
| `flash` | mostra tamanho e velocidade da memória flash |
| `temp` | temperatura interna da CPU |
| `mac` | endereço MAC do Wi‑Fi |
| `net_info` | IP, RSSI e SSID da rede atual |
| `uptime` | tempo de atividade do sistema em segundos |
| `scan` | dispara a varredura dos escravos para verificar online/offline |

### Comandos remotos / UDP usados pela central

A central também envia comandos para escravos via UDP em rede local:

- `alive` — usado pela varredura para detectar escravos online
- `LED_ON`
- `LED_OFF`
- `LED_BLINK:1000`
- `TEMP`
- `CPU`
- `RAM`
- `INFO`
- `STATUS`
- `VERSION`
- `UPTIME`
- `NET_INFO`

> Observação: o texto de ajuda do sistema também menciona `list`, `read:[arquivo]` e `del:[arquivo]`, mas no código atual esses handlers não estão implementados. Os comandos efetivamente processados estão listados acima.

### Exemplo de uso

```bash
help
info
scan
set_fuso:-3
led_blink:800
set_escravo2:192.168.0.125
```
