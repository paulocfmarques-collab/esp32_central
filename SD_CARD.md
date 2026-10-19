# Cartao SD da Central

## Monitoramento Prometheus

O painel permanece na porta **80**. Abra `/prometheus` pelo link no painel,
informe o endereco do servidor, a porta de metricas da Central (padrao **9120**)
e o intervalo sugerido. As configuracoes persistem nas Preferences/NVS
`telemetry`. O servidor coleta `/metrics` na porta salva; nao ha envio ativo.
Baixe o YAML no formulario e aplique no servidor.

O SD exporta montagem, disponibilidade do cache, capacidade fisica, total,
usado/livre, porcentagem de uso, frequencia SPI, tamanho de `/log.txt`,
registros gravados e falhas de log desde boot. A coleta usa somente o cache
numerico protegido pelo mutex SPI, sem acessar ou reconfigurar o cartao.
Se o mutex estiver ocupado, `sdcard_metrics_available=0`; sem cartao pronto,
capacidades/ocupacao sao omitidas, nao preenchidas com zeros ficticios.
Espaco continua atualizado a cada 30 segundos ou por `sd_status`.
Veja o catalogo e o roteiro em [PROMETHEUS.md](PROMETHEUS.md).

A pinagem do conector TF e o mapa dos perifericos integrados estao em
[PINAGEM.md](PINAGEM.md). Para o cartao, use CS 5, CLK 18, MOSI 23 (CMD) e
MISO 19 (DAT0), conforme a tabela junto ao conector TF. Algumas legendas
MISO/MOSI junto ao modulo ESP no diagrama estao invertidas em relacao a
essa tabela.

Display, touch e SD possuem tres grupos de pinos SPI, mas o ESP32 tem dois
controladores disponiveis. O SD alterna temporariamente o controlador do TFT:
trava o mutex do display, desativa os selects, troca os pinos, executa a
operacao e restaura o TFT antes de liberar o mutex. O touch mantem o HSPI.

A montagem tenta 4 MHz, 1 MHz e 400 kHz. Nenhuma tentativa formata o cartao.
Na Arduino IDE, a particao usada e **Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)**.
Ela conserva o OTA; o SD usa seu proprio sistema de arquivos e nao usa o SPIFFS.
Use um cartao com sistema de arquivos FAT32. Se inserir um cartao depois do
boot, aguarde a verificacao de 30 segundos ou envie `sd_remount` pela serial.

## Validacao no dispositivo

Abra a serial em 115200 baud e envie um comando por linha:

```text
sd_status
sd_test
sd_write:/central_teste.txt:Primeira linha
sd_append:/central_teste.txt:Segunda linha
sd_read:/central_teste.txt
sd_list
sd_log
```

`sd_test` cria, le e remove `/central_sd_test.tmp`. Se esse arquivo ja existir,
o teste preserva-o e informa o motivo. O arquivo `/central_teste.txt` do
exemplo permanece no cartao. Para removê-lo, use `sd_del:/central_teste.txt`.

## Log automatico

Todos os comandos processados e suas respostas sao registrados em `/log.txt`:

- `COMANDO_RX`: entrada pela serial, tela ou UDP.
- `COMANDO_TX`: comando enviado pela Central a um ESP por UDP.
- `RESPOSTA_RX`: resposta de ESP recebida e processada pela Central.
- `RESPOSTA_TX`: resposta exibida na tela/serial ou enviada por UDP.
- `FALHA_TX` e `ERRO`: envio malsucedido ou timeout.

Cada linha inclui data/hora local quando o NTP esta sincronizado, uptime,
origem/destino e texto. Quebras de linha, tabs e barras sao escapadas para
manter um evento por linha. O log acrescenta registros e permanece entre
reinicializacoes; nao tem exclusao automatica. `sd_log` retorna os ultimos
1000 bytes, e `sd_clear_log` apaga o arquivo; a resposta dessa operacao inicia
um novo log.

A sexta pagina de diagnosticos mostra capacidade, espaco usado/livre,
frequencia, tamanho do log, registros gravados nesta inicializacao e falhas.
Sem cartao ou com falha de escrita, os comandos continuam funcionando e as
perdas de registro sao contadas. Esses eventos nao sao recuperados depois.
Espaco usado/livre e atualizado a cada 30 segundos ou por `sd_status`.

A pagina web e servida comprimida para economizar Flash. Depois de alterar
`WebPage.h`, execute `node compress-web.cjs` para regenerar `WebPageGzip.h`.

Na pagina web, a aba **Log do SD** le as linhas completas mais recentes de /log.txt (ate 16 KB). A leitura ocorre ao abrir a aba ou pressionar Atualizar log; nao altera o arquivo.

O mesmo `/log.txt` recebe eventos `BOOT`: build, motivo do reset, serial USB,
montagem do SD, conexao Wi-Fi (SSID/IP/RSSI), OTA,
sincronizacao NTP, criacao da tarefa de rede e fim da inicializacao.
Mensagens anteriores a montagem ficam num buffer fixo de ate oito entradas
de 191 caracteres, com o instante original em milissegundos, e sao gravadas
assim que o SD inicia. Antes do NTP, a data aparece como `SEM_NTP`.
Sem SD gravavel, os registros nao persistem. Se houver reset antes de montar
o cartao, o buffer em RAM daquela tentativa e perdido.

`crash_info` arquiva o relatorio de crash em `/crash_AAAAMMDD_HHMMSS.txt`
(com sufixo numerico se necessario), verifica a copia lendo o arquivo e so
depois apaga o registro interno da Flash. Em caso de falha no SD, preserva
o registro interno. Sem NTP, o nome indica `SEM_NTP` e o uptime. Os arquivos
podem ser localizados com `sd_list:/` e consultados com `sd_read:/crash_...txt`.

Apos 5 minutos sem toque, o display permanece ligado com um protetor Matrix. Um toque retorna ao menu principal. Diagnosticos continuam retornando ao menu apos 30 segundos.
