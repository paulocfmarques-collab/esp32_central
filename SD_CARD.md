# Cartao SD da Central

O conector TF da ESP32-2432S028 usa CS 5, CLK 18, MOSI 23 (CMD) e MISO 19
(DAT0), conforme a tabela junto ao conector no manual fornecido. As legendas
MISO/MOSI junto ao modulo ESP do mesmo desenho estao invertidas em relacao
a essa tabela.

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

Apos 5 minutos sem toque, o display permanece ligado com um protetor Matrix. Um toque retorna ao menu principal. Diagnosticos continuam retornando ao menu apos 30 segundos.
