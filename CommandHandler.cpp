#include "CommandHandler.h"
#include <SPI.h>
#include <SD.h>

#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CS   5
#define SD_RESP_MAX 1000

static SPIClass sdSpi(VSPI);
static bool sdPronto = false;

static bool sdMontar() {
    if (sdPronto) return true;
    sdSpi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    sdPronto = SD.begin(SD_CS, sdSpi, 4000000);
    return sdPronto;
}

static bool sdCaminhoValido(const String& p) {
    return p.length() > 0 && p[0] == '/' && p.indexOf("..") < 0;
}

static String sdTipo() {
    switch (SD.cardType()) {
        case CARD_MMC:  return "MMC";
        case CARD_SD:   return "SDSC";
        case CARD_SDHC: return "SDHC";
        default:        return "?";
    }
}

static bool sdRemoverPasta(const String& path) {
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) return false;
    File f = dir.openNextFile();
    while (f) {
        String filho = String(f.path());
        bool isDir = f.isDirectory();
        f.close();
        if (isDir) sdRemoverPasta(filho); else SD.remove(filho);
        f = dir.openNextFile();
    }
    dir.close();
    return SD.rmdir(path);
}

static String executarSd(const String& original, const String& cmd) {
    if (!sdMontar()) return "SD: cartao nao encontrado.";

    // Mant?m a caixa original do argumento (cmd vem em min?sculas)
    String arg = "";
    int dp = original.indexOf(':');
    if (dp >= 0) { arg = original.substring(dp + 1); arg.trim(); }

    if (cmd == "sd_status") {
        uint64_t total = SD.totalBytes() / (1024 * 1024);
        uint64_t usado = SD.usedBytes() / (1024 * 1024);
        return "SD: OK\nTipo: " + sdTipo() + "\nTotal: " + String((uint32_t)total) +
               " MB\nUsado: " + String((uint32_t)usado) + " MB";
    }
    if (cmd == "sd_test") {
        File f = SD.open("/_teste.tmp", FILE_WRITE);
        if (!f) return "SD teste: falha ao criar.";
        f.print("ok");
        f.close();
        f = SD.open("/_teste.tmp");
        String lido = f ? f.readString() : "";
        if (f) f.close();
        SD.remove("/_teste.tmp");
        return lido == "ok" ? "SD teste: OK" : "SD teste: FALHA";
    }
    if (cmd == "sd_log" || cmd.startsWith("sd_read")) {
        String path = (cmd == "sd_log") ? "/log.txt" : arg;
        if (!sdCaminhoValido(path)) return "Uso: sd_read:/arquivo";
        File f = SD.open(path);
        if (!f || f.isDirectory()) return "Arquivo nao encontrado: " + path;
        size_t tam = f.size();
        bool cortou = tam > SD_RESP_MAX;
        if (cortou) f.seek(tam - SD_RESP_MAX);
        String out = f.readString();
        f.close();
        if (cortou) out = "[...]\n" + out;
        return out.length() ? out : "(vazio)";
    }
    if (cmd.startsWith("sd_list")) {
        String path = arg.length() ? arg : "/";
        if (!sdCaminhoValido(path)) return "Uso: sd_list:/pasta";
        File dir = SD.open(path);
        if (!dir || !dir.isDirectory()) return "Pasta nao encontrada: " + path;
        String out = "";
        File f = dir.openNextFile();
        while (f && out.length() < SD_RESP_MAX - 60) {
            out += String(f.name()) + (f.isDirectory() ? "/" : " (" + String((uint32_t)f.size()) + " B)") + "\n";
            f.close();
            f = dir.openNextFile();
        }
        if (f) { f.close(); out += "[...]"; }
        dir.close();
        return out.length() ? out : "(pasta vazia)";
    }
    if (cmd.startsWith("sd_write") || cmd.startsWith("sd_append")) {
        int sep = arg.indexOf(':');
        if (sep < 0) return "Uso: " + cmd.substring(0, cmd.indexOf(':')) + ":/arq:texto";
        String path = arg.substring(0, sep);
        String texto = arg.substring(sep + 1);
        if (!sdCaminhoValido(path)) return "Caminho invalido.";
        bool append = cmd.startsWith("sd_append");
        File f = SD.open(path, append ? FILE_APPEND : FILE_WRITE);
        if (!f) return "Falha ao abrir: " + path;
        if (append) f.println(texto); else f.print(texto);
        f.close();
        return String(append ? "Adicionado em " : "Gravado em ") + path;
    }
    if (cmd.startsWith("sd_del")) {
        if (!sdCaminhoValido(arg)) return "Uso: sd_del:/arquivo";
        return SD.remove(arg) ? "Apagado: " + arg : "Falha ao apagar: " + arg;
    }
    if (cmd.startsWith("sd_mkdir")) {
        if (!sdCaminhoValido(arg)) return "Uso: sd_mkdir:/pasta";
        return SD.mkdir(arg) ? "Pasta criada: " + arg : "Falha ao criar: " + arg;
    }
    if (cmd.startsWith("sd_rmdir")) {
        if (!sdCaminhoValido(arg) || arg == "/") return "Uso: sd_rmdir:/pasta";
        return sdRemoverPasta(arg) ? "Pasta removida: " + arg : "Falha ao remover: " + arg;
    }
    if (cmd == "sd_clear_log") {
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

void CommandHandler::inicializarHardware() {
    pinMode(LED_PINOBLE, OUTPUT);
    digitalWrite(LED_PINOBLE, HIGH); 
    sdMontar();
}

String CommandHandler::obterMotivoReset() {
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason) {
        case ESP_RST_UNKNOWN:   return "DESCONHECIDO";
        case ESP_RST_POWERON:   return "POWER-ON (Tomada/VCC)";
        case ESP_RST_EXT:       return "PINO RESET (Botao EN)";
        case ESP_RST_SW:        return "SOFTWARE / OUTROS";
        case ESP_RST_PANIC:     return "CRASH / PANIC (Exception)";
        case ESP_RST_INT_WDT:   return "WATCHDOG INTERNO (Core Travado)";
        case ESP_RST_TASK_WDT:  return "TASK WATCHDOG (Loop Travado)";
        case ESP_RST_WDT:       return "OUTROS WATCHDOGS";
        case ESP_RST_DEEPSLEEP: return "ACORDOU DO DEEP SLEEP";
        case ESP_RST_BROWNOUT:  return "BROWNOUT (Queda de Tensao)";
        case ESP_RST_SDIO:      return "RESET VIA SDIO";
        default:                return "CODIGO NAO MAPEADO";
    }
}

void CommandHandler::gerenciarBlinkAsync() {
    if (_blinkAtivo) {
        unsigned long atual = millis();
        if (atual - _ultimoToggle >= _intervaloBlink) {
            _ultimoToggle = atual;
            _estadoLed = !_estadoLed;
            digitalWrite(LED_PINOBLE, _estadoLed ? LOW : HIGH);
        }
    }
}

void CommandHandler::executar(const String& cmdBruto, bool requisicaoRemotaUdp) {
    String cmd = cmdBruto;
    cmd.toLowerCase();
    cmd.trim();

    if (cmd != "lastcmd" && cmd != "cmdcount") {
        _mestreUltimoCmd = cmd;
        _mestreTotalCmd++;
    }

    Serial.print(F("Comando Mestre processando: "));
    Serial.println(cmd);
    String resp = "";

    if (cmd == "help") {
        resp = "===== COMANDOS ACEITOS =====\n"
               "help / info / status / reason\n"
               "version / build / cpu / ram\n"
               "flash / temp / mac / net_info\n"
               "uptime / time / date / scan\n"
               "--- CONFIGURACOES ---\n"
               "set_fuso:[num] (Ex: set_fuso:-3)\n"
               "reset_wifi : Limpa Flash e abre AP\n"
               "set_escravo[1-4]:[IP]\n"
               "--- SD CARD ---\n"
               "sd_status / sd_test / sd_log\n"
               "sd_list[:/pasta] / sd_read:/arq\n"
               "sd_write:/arq:txt / sd_append:/arq:txt\n"
               "sd_del:/arq / sd_mkdir:/p / sd_rmdir:/p\n"
               "sd_clear_log";
    }
    else if (cmd == "info") {
        String dataHoraCompleta; ntp.getDateTime(dataHoraCompleta, 100);
        String dataStr = "Sem Sinc.", horaStr = "--:--:--";
        if (dataHoraCompleta.length() >= 19) {
            dataStr = dataHoraCompleta.substring(0, 10);
            horaStr = dataHoraCompleta.substring(11, 19);
        }
        uint32_t heapLivre = ESP.getFreeHeap() / 1024;
        float flashLivre = (float)ESP.getFreeSketchSpace() / (1024.0 * 1024.0);

        resp = "===== DEVICE INFO =====\n"
            "Hostname: ESP32_CENTRAL\n"
            "Firmware: " + obterVersaoAutomatica() + "\n"
            "Build: " + String(__DATE__) + " " + String(__TIME__) + "\n" +
            "SSID: " + WiFi.SSID() + "\n" +
            "IP: " + WiFi.localIP().toString() + "\n" +
            "MAC: " + WiFi.macAddress() + "\n" +
            "RSSI: " + String(WiFi.RSSI()) + " dBm\n" +
            "Heap Livre: " + String(heapLivre) + " KB\n" +
            "Flash Livre: " + String(flashLivre, 1) + " MB\n" +
            "SD Card: " + String(sdPronto ? "OK" : "N/A") + "\n" + 
            "Data: " + dataStr + "\n" +
            "Hora: " + horaStr + "\n" +
            "Uptime: " + String(millis()) + " ms\n" +
            "Reset: " + obterMotivoReset() + "\n" + 
            "=======================";
    }
    else if (cmd == "reason") {
        resp = "===== ULTIMO RESET =====\n"
               "Motivo: " + obterMotivoReset() + "\n"
               "Uptime Atual: " + String(millis() / 1000) + " s\n"
               "========================";
    }
    else if (cmd.startsWith("sd_")) {
        resp = executarSd(cmdBruto, cmd);
    }
    else if (cmd == "vago") {
        resp = "Central Local:\nEste slot de comando esta vazio.";
    }
    else if (cmd == "version") {
        resp = "Central Mestre:\nVersao Firmware: " + obterVersaoAutomatica();
    }
    else if (cmd == "build") {
        resp = "Central Mestre Build:\nData: " + String(__DATE__) + "\nHora: " + String(__TIME__);
    }
    else if (cmd == "status") {
        String statusWifi = (WiFi.status() == WL_CONNECTED) ? "OK" : "FALHA";
        uint32_t heapKB = ESP.getFreeHeap() / 1024;
        resp = "ONLINE (MESTRE)\nWiFi: " + statusWifi + 
               "\nSD: " + String(sdPronto ? "OK" : "N/A") + 
               "\nNTP: " + (ntp.isSincronizado() ? "OK" : "FALHA") + 
               "\nHeap: " + String(heapKB) + " KB\n";
    }
    else if (cmd == "lastcmd") {
        resp = "Mestre Ultimo Cmd:\n" + _mestreUltimoCmd + "\n";
    }
    else if (cmd == "cmdcount") {
        resp = "Mestre Cmd Count:\nTotal: " + String(_mestreTotalCmd) + " processados\n";
    }
    else if (cmd.startsWith("set_escravo1:")) {
        String novoIp = cmd.substring(13); novoIp.trim();
        udp.atualizarIpEscravo(1, novoIp);
        resp = "Escravo 1 IP:\n" + novoIp + "\n";
        central.renderizarTela();
    }
    else if (cmd.startsWith("set_escravo2:")) {
        String novoIp = cmd.substring(13); novoIp.trim();
        udp.atualizarIpEscravo(2, novoIp);
        resp = "Escravo 2 IP:\n" + novoIp + "\n";
        central.renderizarTela();
    }
    else if (cmd.startsWith("set_escravo3:")) {
        String novoIp = cmd.substring(13); novoIp.trim();
        udp.atualizarIpEscravo(3, novoIp);
        resp = "Escravo 3 IP:\n" + novoIp + "\n";
        central.renderizarTela();
    }
    else if (cmd.startsWith("set_escravo4:")) {
        String novoIp = cmd.substring(13); novoIp.trim();
        udp.atualizarIpEscravo(4, novoIp);
        resp = "Escravo 4 IP:\n" + novoIp + "\n";
        central.renderizarTela();
    }
    else if (cmd == "reset_wifi") {
         central.exibirTelaResposta("Limpando Flash...\nSistema reiniciando.");
         Preferences prefs; prefs.begin("wifi", false); prefs.clear(); prefs.end();
         delay(2000); ESP.restart();
    }
    else if (cmd == "desligar") {
        central.exibirTelaResposta("Desligando Sistema...\nEntrando em modo Economia.");
        delay(2000);
        central.desligarDisplayFisico();
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); 
        esp_deep_sleep_start();
    }
    else if (cmd == "led_on") {
        _blinkAtivo = false; _estadoLed = true;
        digitalWrite(LED_PINOBLE, LOW); 
        resp = "Central Local:\nLED ligado com sucesso.\n";
    }
    else if (cmd == "led_off") {
        _blinkAtivo = false; _estadoLed = false;
        digitalWrite(LED_PINOBLE, HIGH); 
        resp = "Central Local:\nLED desligado.\n";
    }
    else if (cmd.startsWith("led_blink:")) {
        _intervaloBlink = cmd.substring(10).toInt();
        if (_intervaloBlink <= 0) _intervaloBlink = 500;
        _blinkAtivo = true;
        resp = "Central Local:\nBlink ativo (" + String(_intervaloBlink) + " ms)\n  ";
    }
    else if (cmd.startsWith("set_fuso:")) {
        int novoFuso = cmd.substring(9).toInt();
        if (novoFuso >= -12 && novoFuso <= 14) {
            ntp.configurarRelogio(novoFuso, false);
            resp = "Fuso alterado:\nGMT " + String(novoFuso) + "\n";
        } else {
            resp = "Erro: Fuso invalido\n";
        }
    }
    else if (cmd == "time") {
        resp = "Hora:\n" + ntp.getSomenteHora() + "\n";
    }
    else if (cmd == "date") {
        String dataHoraCompleta; ntp.getDateTime(dataHoraCompleta, 100);
        String data = (dataHoraCompleta.length() >= 10) ? dataHoraCompleta.substring(0, 10) : "Erro NTP";
        resp = "Data:\n" + data + "\n";
    }
    else if (cmd == "cpu") {
        resp = "Central Local CPU:\nModelo: " + String(ESP.getChipModel()) + "\n" +
               "Revision: " + String(ESP.getChipRevision()) + "\n" +
               "Cores: " + String(ESP.getChipCores()) + "\n" +
               "Freq: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
    }
    else if (cmd == "ram") {
        resp = "Central Local RAM:\nHeap Total: " + String(ESP.getHeapSize() / 1024.0) + " KB\n" +
               "Heap Livre: " + String(ESP.getFreeHeap() / 1024.0) + " KB\n" +
               "Menor Bloco Livre: " + String(ESP.getMinFreeHeap() / 1024.0) + " KB\n" +
               "Maior Bloco Livre: " + String(ESP.getMaxAllocHeap() / 1024.0) + " KB\n" +
               "RAM Utilizada: " + String(100.0 * (ESP.getHeapSize() - ESP.getFreeHeap()) / ESP.getHeapSize()) + " %\n";
    }
    else if (cmd == "flash") {
        resp = "Central Local FLASH:\nTamanho: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB\n" +
               "Velocidade: " + String(ESP.getFlashChipSpeed() / 1000 / 1000) + " MHz\n" + 
               "Flash Mode: " + String(ESP.getFlashChipMode()) + "\n" +
               "Sketch Size: " + String(ESP.getSketchSize() / 1024 / 1024) + " MB\n" +
               "Free Sketch Space: " + String(ESP.getFreeSketchSpace() / 1024 / 1024) + " MB\n" +
               "Flash livre: " + String(100.0 * (ESP.getFlashChipSize() - ESP.getFreeSketchSpace()) / ESP.getFlashChipSize()) + " %\n";
    }
    else if (cmd == "temp") {
        resp = "Central Local:\nCPU Temp: " + String(temperatureRead(), 2) + " C\n";
    }
    else if (cmd == "mac") {
        resp = "Central Local:\nMAC: " + WiFi.macAddress() + "\n";
    }
    else if (cmd == "net_info") {
        resp = "Central Local NET:\nSSID: " + WiFi.SSID() + "\n" +
               "IP: " + WiFi.localIP().toString() + "\n" +
               "Gateway: " + WiFi.gatewayIP().toString() + "\n" +
               "Subnet: " + WiFi.subnetMask().toString() + "\n" +
               "DNS1: " + WiFi.dnsIP(0).toString() + "\n" +
               "DNS2: " + WiFi.dnsIP(1).toString() + "\n" +
               "RSSI: " + String(WiFi.RSSI()) + " dBm\n";
    }
    else if (cmd == "uptime") {
        resp = "Uptime:\n" + String(millis() / 1000) + " s\n";
    }
    else if (cmd == "scan") {
        central.imprimirRodape("Disparando Varredura...", TFT_YELLOW);
        for(int i = 1; i <= 4; i++) {
            LayoutDatabase::statusEscravos[i] = false;
        }
        central.renderizarTela();
        for (int i = 1; i <= 4; i++) {
            udp.enviarComando("alive", i); 
            delay(50); 
        }
        udp.resetarEspera();
        resp = "Varredura Concluida!\nCheque os status na tela home.\n";
        central.renderizarTela();
    }
    else if (cmd == "psram") {
        bool psramPresente = ESP.getPsramSize() > 0;
        resp = "Central Local:\nPSRAM Presente: " + String(psramPresente ? "SIM" : "NAO") + 
               "\nTamanho PSRAM: " + String(ESP.getPsramSize() / 1024 / 1024) + " MB" +
               "\nPSRAM Livre: " + String(ESP.getFreePsram() / 1024 / 1024) + " MB" +
               "\nMaior Bloco Livre PSRAM: " + String(ESP.getMaxAllocPsram() / 1024 / 1024) + " MB" +
               "\nPSRAM Utilizada: " + String(100.0 * (ESP.getPsramSize() - ESP.getFreePsram()) / ESP.getPsramSize()) + " %\n";
    }
    else {
        resp = "Central Local:\nComando desconhecido.";
    }

    central.exibirTelaResposta(resp);
    if (requisicaoRemotaUdp) {
        udp.responderRemoto(resp);
    }
}
