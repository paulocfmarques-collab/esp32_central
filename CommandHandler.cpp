#include "CommandHandler.h"
#include "MetricsService.h"
#include "SdManager.h"
#include "CrashDiagnostics.h"
#include <esp_heap_caps.h>
#define SD_RESP_MAX 1000

static bool sdCaminhoValido(const String& p)
{
    return p.length() > 0 && p[0] == '/' && p.indexOf("..") < 0;
}

static String sdTipo()
{
    switch (SD.cardType())
    {
    case CARD_MMC:
        return "MMC";
    case CARD_SD:
        return "SDSC";
    case CARD_SDHC:
        return "SDHC";
    default:
        return "?";
    }
}

static bool sdRemoverPasta(const String& path)
{
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory())
        return false;
    File f = dir.openNextFile();
    while (f)
    {
        String filho = String(f.path());
        bool isDir = f.isDirectory();
        f.close();
        if (isDir)
            sdRemoverPasta(filho);
        else
            SD.remove(filho);
        f = dir.openNextFile();
    }
    dir.close();
    return SD.rmdir(path);
}

static String executarSd(const String& original, const String& cmd)
{
    SdManager::Acesso acesso(cmd == "sd_remount");
    if (!acesso.pronto())
        return "SD: " + SdManager::snapshot().erro + "\nUse cartao FAT32 e tente sd_remount.";
    String operacao = cmd.substring(0, cmd.indexOf(':') < 0 ? cmd.length() : cmd.indexOf(':'));

    // Preserva caixa e conteudo dos argumentos.
    String arg = "";
    int dp = original.indexOf(':');
    if (dp >= 0)
    {
        arg = original.substring(dp + 1);
        arg.trim();
    }

    if (operacao == "sd_remount")
        return "SD: remontado com sucesso.";
    if (operacao == "sd_status")
    {
        SdManager::atualizarEstatisticas();
        uint64_t total = SD.totalBytes() / (1024 * 1024);
        uint64_t usado = SD.usedBytes() / (1024 * 1024);
        return "SD: OK\nTipo: " + sdTipo() + "\nTotal: " + String((uint32_t)total) +
               " MB\nUsado: " + String((uint32_t)usado) + " MB";
    }
    if (operacao == "sd_test")
    {
        const char* teste = "/central_sd_test.tmp";
        if (SD.exists(teste))
            return "SD teste: arquivo temporario ja existe; preservado.";
        File f = SD.open(teste, FILE_WRITE);
        if (!f)
            return "SD teste: falha ao criar.";
        size_t escritos = f.print("ok");
        f.flush();
        f.close();
        f = SD.open(teste);
        String lido = f ? f.readString() : "";
        if (f)
            f.close();
        bool removido = SD.remove(teste);
        bool ok = escritos == 2 && lido == "ok" && removido;
        if (!ok)
            SdManager::falha("Teste de leitura/gravação falhou");
        return ok ? "SD teste: OK (gravacao, leitura e remocao)" : "SD teste: FALHA";
    }
    if (operacao == "sd_log" || operacao == "sd_read")
    {
        String path = (cmd == "sd_log") ? "/log.txt" : arg;
        if (!sdCaminhoValido(path))
            return "Uso: sd_read:/arquivo";
        File f = SD.open(path);
        if (!f || f.isDirectory())
            return "Arquivo nao encontrado: " + path;
        size_t tam = f.size();
        bool cortou = tam > SD_RESP_MAX;
        if (cortou)
            f.seek(tam - SD_RESP_MAX);
        char buffer[SD_RESP_MAX + 1];
        size_t lidos = f.readBytes(buffer, SD_RESP_MAX);
        buffer[lidos] = '\0';
        String out(buffer);
        f.close();
        if (cortou)
            out = "[...]\n" + out;
        return out.length() ? out : "(vazio)";
    }
    if (operacao == "sd_list")
    {
        String path = arg.length() ? arg : "/";
        if (!sdCaminhoValido(path))
            return "Uso: sd_list:/pasta";
        File dir = SD.open(path);
        if (!dir || !dir.isDirectory())
            return "Pasta nao encontrada: " + path;
        String out = "";
        File f = dir.openNextFile();
        while (f && out.length() < SD_RESP_MAX - 60)
        {
            out += String(f.name()) +
                   (f.isDirectory() ? "/" : " (" + String((uint32_t)f.size()) + " B)") + "\n";
            f.close();
            f = dir.openNextFile();
        }
        if (f)
        {
            f.close();
            out += "[...]";
        }
        dir.close();
        return out.length() ? out : "(pasta vazia)";
    }
    if (operacao == "sd_write" || operacao == "sd_append")
    {
        int sep = arg.indexOf(':');
        if (sep < 0)
            return "Uso: " + cmd.substring(0, cmd.indexOf(':')) + ":/arq:texto";
        String path = arg.substring(0, sep);
        String texto = arg.substring(sep + 1);
        if (!sdCaminhoValido(path))
            return "Caminho invalido.";
        bool append = cmd.startsWith("sd_append");
        File f = SD.open(path, append ? FILE_APPEND : FILE_WRITE);
        if (!f)
            return "Falha ao abrir: " + path;
        size_t escritos = append ? f.println(texto) : f.print(texto);
        size_t esperado = texto.length() + (append ? 2 : 0);
        f.flush();
        f.close();
        if (escritos != esperado)
        {
            SdManager::falha("Falha de escrita / cartao cheio");
            return "SD: gravacao incompleta.";
        }
        return String(append ? "Adicionado em " : "Gravado em ") + path;
    }
    if (operacao == "sd_del")
    {
        if (!sdCaminhoValido(arg))
            return "Uso: sd_del:/arquivo";
        return SD.remove(arg) ? "Apagado: " + arg : "Falha ao apagar: " + arg;
    }
    if (operacao == "sd_mkdir")
    {
        if (!sdCaminhoValido(arg))
            return "Uso: sd_mkdir:/pasta";
        return SD.mkdir(arg) ? "Pasta criada: " + arg : "Falha ao criar: " + arg;
    }
    if (operacao == "sd_rmdir")
    {
        if (!sdCaminhoValido(arg) || arg == "/")
            return "Uso: sd_rmdir:/pasta";
        return sdRemoverPasta(arg) ? "Pasta removida: " + arg : "Falha ao remover: " + arg;
    }
    if (operacao == "sd_clear_log")
    {
        return SD.remove("/log.txt") ? "Log apagado." : "Sem log para apagar.";
    }
    return "Comando SD desconhecido.";
}

// Alocação física das propriedades privadas da classe
bool CommandHandler::_blinkAtivo = false;
bool CommandHandler::_estadoLed = false;
unsigned long CommandHandler::_ultimoToggle = 0;
unsigned long CommandHandler::_intervaloBlink = 500;
String CommandHandler::_mestreUltimoCmd = "nenhum";
uint32_t CommandHandler::_mestreTotalCmd = 0;

void CommandHandler::inicializarHardware()
{
    pinMode(LED_PINOBLE, OUTPUT);
    digitalWrite(LED_PINOBLE, HIGH);
    SdManager::inicializar();
}

String CommandHandler::obterMotivoReset()
{
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason)
    {
    case ESP_RST_UNKNOWN:
        return "DESCONHECIDO";
    case ESP_RST_POWERON:
        return "POWER-ON (Tomada/VCC)";
    case ESP_RST_EXT:
        return "PINO RESET (Botao EN)";
    case ESP_RST_SW:
        return "SOFTWARE / OUTROS";
    case ESP_RST_PANIC:
        return "CRASH / PANIC (Exception)";
    case ESP_RST_INT_WDT:
        return "WATCHDOG INTERNO (Core Travado)";
    case ESP_RST_TASK_WDT:
        return "TASK WATCHDOG (Loop Travado)";
    case ESP_RST_WDT:
        return "OUTROS WATCHDOGS";
    case ESP_RST_DEEPSLEEP:
        return "ACORDOU DO DEEP SLEEP";
    case ESP_RST_BROWNOUT:
        return "BROWNOUT (Queda de Tensao)";
    case ESP_RST_SDIO:
        return "RESET VIA SDIO";
    default:
        return "CODIGO NAO MAPEADO";
    }
}

void CommandHandler::gerenciarBlinkAsync()
{
    if (_blinkAtivo)
    {
        unsigned long atual = millis();
        if (atual - _ultimoToggle >= _intervaloBlink)
        {
            _ultimoToggle = atual;
            _estadoLed = !_estadoLed;
            digitalWrite(LED_PINOBLE, _estadoLed ? LOW : HIGH);
        }
    }
}

void CommandHandler::executar(const String& cmdBruto, bool requisicaoRemotaUdp, const char* origem)
{
    String cmd = cmdBruto;
    cmd.toLowerCase();
    cmd.trim();
    if (!cmd.length())
        return;
    String origemLog = String(origem);
    if (requisicaoRemotaUdp && origemLog == "tela")
        origemLog = "udp:" + udp.obterIpRemotoReal();
    SdManager::registrar("COMANDO_RX", origemLog, cmdBruto);

    if (cmd != "lastcmd" && cmd != "cmdcount")
    {
        _mestreUltimoCmd = cmd;
        _mestreTotalCmd++;
    }

    Serial.print(F("Comando Mestre processando: "));
    Serial.println(cmd);
    String resp = "";

    if (cmd == "help")
    {
        resp = "COMANDOS\n"
               "help\n"
               "name\n"
               "info\n"
               "status\n"
               "reason\n"
               "crash_info\n"
               "version\n"
               "build\n"
               "uptime\n"
               "lastcmd\n"
               "cmdcount\n"
               "reset_counters\n"
               "vago\n"
               "reboot\n"
               "reset_wifi\n"
               "desligar\n"
               "cpu\n"
               "ram\n"
               "metrics_diag\n"
               "flash\n"
               "temp\n"
               "psram\n"
               "mac\n"
               "net_info\n"
               "scan\n"
               "time\n"
               "date\n"
               "set_fuso:N\n"
               "led_on\n"
               "led_off\n"
               "led_blink:ms\n"
               "set_escravo1:IP\n"
               "set_escravo2:IP\n"
               "set_escravo3:IP\n"
               "set_escravo4:IP\n"
               "sd_status\n"
               "sd_test\n"
               "sd_log\n"
               "sd_remount\n"
               "sd_list:[pasta]\n"
               "sd_read:arquivo\n"
               "sd_write:arquivo:texto\n"
               "sd_append:arquivo:texto\n"
               "sd_del:arquivo\n"
               "sd_mkdir:pasta\n"
               "sd_rmdir:pasta\n"
               "sd_clear_log\n";
    }
    else if (cmd == "reset_counters")
    {
        resp = MetricsService::resetCounters()
                   ? "Contadores NVS de boot/watchdog zerados.\n"
                   : "Erro ao zerar contadores NVS.\n";
    }
    else if (cmd == "reboot")
    {
        SdManager::registrar("RESPOSTA_TX", origemLog, "Fazendo reboot...");
        Serial.println(F("[RESPOSTA]\nfazendo reboot...\n[FIM RESPOSTA]"));
        central.exibirTelaResposta("Inicializando o sistema...\n");
        delay(2000);
        ESP.restart();
    }
    else if (cmd == "info")
    {
        String dataHoraCompleta;
        ntp.getDateTime(dataHoraCompleta, 100);
        String dataStr = "Sem Sinc.", horaStr = "--:--:--";
        if (dataHoraCompleta.length() >= 19)
        {
            dataStr = dataHoraCompleta.substring(0, 10);
            horaStr = dataHoraCompleta.substring(11, 19);
        }
        uint32_t heapLivre = ESP.getFreeHeap() / 1024;
        float flashLivre = (float)ESP.getFreeSketchSpace() / (1024.0 * 1024.0);

        resp = "===== DEVICE INFO =====\n"
               "Hostname: ESP32_CENTRAL\n"
               "Firmware: " +
               obterVersaoAutomatica() +
               "\n"
               "Build: " +
               String(__DATE__) + " " + String(__TIME__) + "\n" + "SSID: " + WiFi.SSID() + "\n" +
               "IP: " + WiFi.localIP().toString() + "\n" + "MAC: " + WiFi.macAddress() + "\n" +
               "RSSI: " + String(WiFi.RSSI()) + " dBm\n" + "Heap Livre: " + String(heapLivre) +
               " KB\n" + "Flash Livre: " + String(flashLivre, 1) + " MB\n" +
               "SD Card: " + String(SdManager::snapshot().pronto ? "OK" : "N/A") + "\n" +
               "Data: " + dataStr + "\n" + "Hora: " + horaStr + "\n" +
               "Uptime: " + String(millis()) + " ms\n" + "Reset: " + obterMotivoReset() + "\n" +
               "=======================";
    }
    else if (cmd == "crash_info")
    {
        resp = CrashDiagnostics::lerArquivarEApagar();
    }
    else if (cmd == "reason")
    {
        resp = "===== ULTIMO RESET =====\n"
               "Motivo: " +
               obterMotivoReset() +
               "\n"
               "Uptime Atual: " +
               String(millis() / 1000) +
               " s\n"
               "========================";
    }
    else if (cmd.startsWith("sd_"))
    {
        resp = executarSd(cmdBruto, cmd);
    }
    else if (cmd == "vago")
    {
        resp = "Central Local:\nEste slot de comando esta vazio.";
    }
    else if (cmd == "version")
    {
        resp = "Central Mestre:\nVersao Firmware: " + obterVersaoAutomatica();
    }
    else if (cmd == "build")
    {
        resp = "Central Mestre Build:\nData: " + String(__DATE__) + "\nHora: " + String(__TIME__);
    }
    else if (cmd == "status")
    {
        String statusWifi = (WiFi.status() == WL_CONNECTED) ? "OK" : "FALHA";
        uint32_t heapKB = ESP.getFreeHeap() / 1024;
        resp = "ONLINE (MESTRE)\nWiFi: " + statusWifi +
               "\nSD: " + String(SdManager::snapshot().pronto ? "OK" : "N/A") +
               "\nNTP: " + (ntp.isSincronizado() ? "OK" : "FALHA") + "\nHeap: " + String(heapKB) +
               " KB\n";
    }
    else if (cmd == "lastcmd")
    {
        resp = "Mestre Ultimo Cmd:\n" + _mestreUltimoCmd + "\n";
    }
    else if (cmd == "cmdcount")
    {
        resp = "Mestre Cmd Count:\nTotal: " + String(_mestreTotalCmd) + " processados\n";
    }
    else if (cmd.startsWith("set_escravo1:"))
    {
        String novoIp = cmd.substring(13);
        novoIp.trim();
        udp.atualizarIpEscravo(1, novoIp);
        resp = "Escravo 1 IP:\n" + novoIp + "\n";
        central.renderizarTela();
    }
    else if (cmd.startsWith("set_escravo2:"))
    {
        String novoIp = cmd.substring(13);
        novoIp.trim();
        udp.atualizarIpEscravo(2, novoIp);
        resp = "Escravo 2 IP:\n" + novoIp + "\n";
        central.renderizarTela();
    }
    else if (cmd.startsWith("set_escravo3:"))
    {
        String novoIp = cmd.substring(13);
        novoIp.trim();
        udp.atualizarIpEscravo(3, novoIp);
        resp = "Escravo 3 IP:\n" + novoIp + "\n";
        central.renderizarTela();
    }
    else if (cmd.startsWith("set_escravo4:"))
    {
        String novoIp = cmd.substring(13);
        novoIp.trim();
        udp.atualizarIpEscravo(4, novoIp);
        resp = "Escravo 4 IP:\n" + novoIp + "\n";
        central.renderizarTela();
    }
    else if (cmd == "reset_wifi")
    {
        SdManager::registrar("RESPOSTA_TX", origemLog, "Limpando Wi-Fi. Reiniciando...");
        Serial.println(F("[RESPOSTA]\nLimpando Wi-Fi. Reiniciando...\n[FIM RESPOSTA]"));
        central.exibirTelaResposta("Limpando Flash...\nSistema reiniciando.");
        Preferences prefs;
        prefs.begin("wifi", false);
        prefs.clear();
        prefs.end();
        delay(2000);
        ESP.restart();
    }
    else if (cmd == "desligar")
    {
        SdManager::registrar("RESPOSTA_TX", origemLog, "Entrando em deep sleep...");
        Serial.println(F("[RESPOSTA]\nEntrando em deep sleep...\n[FIM RESPOSTA]"));
        central.exibirTelaResposta("Desligando Sistema...\nEntrando em modo Economia.");
        delay(2000);
        central.desligarDisplayFisico();
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
        esp_deep_sleep_start();
    }
    else if (cmd == "led_on")
    {
        _blinkAtivo = false;
        _estadoLed = true;
        digitalWrite(LED_PINOBLE, LOW);
        resp = "Central Local:\nLED ligado com sucesso.\n";
    }
    else if (cmd == "led_off")
    {
        _blinkAtivo = false;
        _estadoLed = false;
        digitalWrite(LED_PINOBLE, HIGH);
        resp = "Central Local:\nLED desligado.\n";
    }
    else if (cmd.startsWith("led_blink:"))
    {
        _intervaloBlink = cmd.substring(10).toInt();
        if (_intervaloBlink <= 0)
            _intervaloBlink = 500;
        _blinkAtivo = true;
        resp = "Central Local:\nBlink ativo (" + String(_intervaloBlink) + " ms)\n  ";
    }
    else if (cmd.startsWith("set_fuso:"))
    {
        int novoFuso = cmd.substring(9).toInt();
        if (novoFuso >= -12 && novoFuso <= 14)
        {
            ntp.configurarRelogio(novoFuso, false);
            resp = "Fuso alterado:\nGMT " + String(novoFuso) + "\n";
        }
        else
        {
            resp = "Erro: Fuso invalido\n";
        }
    }
    else if (cmd == "time")
    {
        resp = "Hora:\n" + ntp.getSomenteHora() + "\n";
    }
    else if (cmd == "date")
    {
        String dataHoraCompleta;
        ntp.getDateTime(dataHoraCompleta, 100);
        String data =
            (dataHoraCompleta.length() >= 10) ? dataHoraCompleta.substring(0, 10) : "Erro NTP";
        resp = "Data:\n" + data + "\n";
    }
    else if (cmd == "cpu")
    {
        resp = "Central Local CPU:\nModelo: " + String(ESP.getChipModel()) + "\n" +
               "Revision: " + String(ESP.getChipRevision()) + "\n" +
               "Cores: " + String(ESP.getChipCores()) + "\n" +
               "Freq: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
    }
    else if (cmd == "metrics_diag")
    {
        resp = MetricsService::diagnostico();
    }
    else if (cmd == "ram")
    {
        resp = "Central Local RAM:\nHeap Total: " + String(ESP.getHeapSize() / 1024.0) + " KB\n" +
               "Heap Livre: " + String(ESP.getFreeHeap() / 1024.0) + " KB\n" +
               "Menor Bloco Livre: " + String(ESP.getMinFreeHeap() / 1024.0) + " KB\n" +
               "Maior Bloco Livre: " + String(ESP.getMaxAllocHeap() / 1024.0) + " KB\n" +
               "Heap 8 bits livre: " + String(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) + " bytes\n" +
               "Maior bloco 8 bits: " + String(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) + " bytes\n" +
               "RAM Utilizada: " +
               String(100.0 * (ESP.getHeapSize() - ESP.getFreeHeap()) / ESP.getHeapSize()) + " %\n";
    }
    else if (cmd == "flash")
    {
        resp = "Central Local FLASH:\nTamanho: " + String(ESP.getFlashChipSize() / 1024 / 1024) +
               " MB\n" + "Velocidade: " + String(ESP.getFlashChipSpeed() / 1000 / 1000) + " MHz\n" +
               "Flash Mode: " + String(ESP.getFlashChipMode()) + "\n" +
               "Sketch Size: " + String(ESP.getSketchSize() / 1024 / 1024) + " MB\n" +
               "Free Sketch Space: " + String(ESP.getFreeSketchSpace() / 1024 / 1024) + " MB\n" +
               "Flash livre: " +
               String(100.0 * (ESP.getFlashChipSize() - ESP.getFreeSketchSpace()) /
                      ESP.getFlashChipSize()) +
               " %\n";
    }
    else if (cmd == "temp")
    {
        resp = "Central Local:\nCPU Temp: " + String(temperatureRead(), 2) + " C\n";
    }
    else if (cmd == "mac")
    {
        resp = "Central Local:\nMAC: " + WiFi.macAddress() + "\n";
    }
    else if (cmd == "net_info")
    {
        resp = "Central Local NET:\nSSID: " + WiFi.SSID() + "\n" +
               "IP: " + WiFi.localIP().toString() + "\n" +
               "Gateway: " + WiFi.gatewayIP().toString() + "\n" +
               "Subnet: " + WiFi.subnetMask().toString() + "\n" +
               "DNS1: " + WiFi.dnsIP(0).toString() + "\n" + "DNS2: " + WiFi.dnsIP(1).toString() +
               "\n" + "RSSI: " + String(WiFi.RSSI()) + " dBm\n";
    }
    else if (cmd == "uptime")
    {
        resp = "Uptime:\n" + String(millis() / 1000) + " s\n";
    }
    else if (cmd == "scan")
    {
        central.imprimirRodape("Disparando Varredura...", TFT_YELLOW);
        for (int i = 1; i <= 4; i++)
        {
            LayoutDatabase::statusEscravos[i] = false;
        }
        central.renderizarTela();
        for (int i = 1; i <= 4; i++)
        {
            udp.enviarComando("alive", i);
            delay(50);
        }
        udp.resetarEspera();
        resp = "Varredura Concluida!\nCheque os status na tela home.\n";
        central.renderizarTela();
    }
    else if (cmd == "psram")
    {
        bool psramPresente = ESP.getPsramSize() > 0;
        resp = "Central Local:\nPSRAM Presente: " + String(psramPresente ? "SIM" : "NAO") +
               "\nTamanho PSRAM: " + String(ESP.getPsramSize() / 1024 / 1024) + " MB" +
               "\nPSRAM Livre: " + String(ESP.getFreePsram() / 1024 / 1024) + " MB" +
               "\nMaior Bloco Livre PSRAM: " + String(ESP.getMaxAllocPsram() / 1024 / 1024) +
               " MB" + "\nPSRAM Utilizada: " +
               String(100.0 * (ESP.getPsramSize() - ESP.getFreePsram()) / ESP.getPsramSize()) +
               " %\n";
    }
    else if (cmd == "name")
    {
        resp = "ESP32_Central\n";
    }
    else
    {
        resp = "Central Local:\nComando desconhecido.";
    }

    Serial.println(F("[RESPOSTA]"));
    SdManager::registrar("RESPOSTA_TX", String("serial+tela/") + origemLog, resp);
    Serial.println(resp);
    Serial.println(F("[FIM RESPOSTA]"));
    central.exibirTelaResposta(resp);
    if (requisicaoRemotaUdp)
    {
        udp.responderRemoto(resp);
    }
}
