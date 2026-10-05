#include "CommandHandler.h"

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
               "--- REMOTOS SD ---\n"
               "list / read:[arq] / del:[arq]";
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
            "SD Card: N/A\n" + 
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
               "\nSD: N/A" + 
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
