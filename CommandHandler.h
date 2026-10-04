#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include "esp_system.h"
#include "InterfaceCentral.h"
#include "UdpComm.h"
#include "NTPUtil.h"
#include <esp_task_wdt.h>

#ifdef __cplusplus
extern "C" {
#endif
float temperatureRead(); 
#ifdef __cplusplus
}
#endif

extern InterfaceCentral central;
extern UdpComm udp;
extern NTPUtil ntp;

#define LED_PINOBLE 4

static String _mestreUltimoCmd = "nenhum";
static uint32_t _mestreTotalCmd = 0;

class OtaManager {
public:
    static volatile int porcentagemAtual;
    static volatile bool otaSolicitado;

    static void inicializar(InterfaceCentral& IC) {
        ArduinoOTA.setHostname("ESP32_CENTRAL");
        ArduinoOTA.setPassword("123456"); 
        porcentagemAtual = -1;
        otaSolicitado = false;
        
        ArduinoOTA.onStart([]() {
            otaSolicitado = true;

            #if ESP_IDF_VERSION_MAJOR >= 5 || defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
                esp_task_wdt_delete(xTaskGetCurrentTaskHandle());
            #else
                esp_task_wdt_delete(NULL);
            #endif

            udp.resetarEspera(); 
            porcentagemAtual = 0;
            Serial.println(F("[Core 0] Chamada OTA aceita. Handshake OK."));
        });
        
        ArduinoOTA.onEnd([]() {
            porcentagemAtual = 100;
            Serial.println(F("[Core 0] Gravacao terminada!"));
            
            #if ESP_IDF_VERSION_MAJOR >= 5 || defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
                esp_task_wdt_add(xTaskGetCurrentTaskHandle());
            #else
                esp_task_wdt_add(NULL);
            #endif
        });
        
        ArduinoOTA.onProgress([](unsigned int progresso, unsigned int total) {
            porcentagemAtual = progresso / (total / 100);
        });
        
        ArduinoOTA.onError([&IC](ota_error_t erro) {
            porcentagemAtual = -1;
            otaSolicitado = false;
            IC.otaGravando = false; 
            Serial.printf("[Core 0] Erro OTA: %d\n", erro);
            
            #if ESP_IDF_VERSION_MAJOR >= 5 || defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
                esp_task_wdt_add(xTaskGetCurrentTaskHandle());
            #else
                esp_task_wdt_add(NULL);
            #endif
        });
        
        ArduinoOTA.begin();
    }
    static void processar() { ArduinoOTA.handle(); }
};

volatile int OtaManager::porcentagemAtual = -1;
volatile bool OtaManager::otaSolicitado = false;

class CommandHandler {
private:
    static bool _blinkAtivo;
    static bool _estadoLed;
    static unsigned long _ultimoToggle;
    static unsigned long _intervaloBlink;

public:
    static void inicializarHardware() {
        pinMode(LED_PINOBLE, OUTPUT);
        digitalWrite(LED_PINOBLE, HIGH); 
    }

    static String obterMotivoReset() {
        esp_reset_reason_t reason = esp_reset_reason();
        switch (reason) {
            case ESP_RST_UNKNOWN:   return "DESCONHECIDO";
            case ESP_RST_POWERON:   return "POWER-ON (Tomada/VCC)";
            case ESP_RST_EXT:       return "PINO RESET (Botao EN)";
            case ESP_RST_SW:        return "SOFTWARE / OUTROS (Restart)";
            case ESP_RST_PANIC:     return "CRASH / PANIC (Exception)";
            case ESP_RST_INT_WDT:   return "WATCHDOG INTERNO (Core Travado)";
            case ESP_RST_TASK_WDT:  return "TASK WATCHDOG (Fila/Loop Travado)";
            case ESP_RST_WDT:       return "OUTROS WATCHDOGS";
            case ESP_RST_DEEPSLEEP: return "ACORDOU DO DEEP SLEEP";
            case ESP_RST_BROWNOUT:  return "BROWNOUT (Queda de Tensao)";
            case ESP_RST_SDIO:      return "RESET VIA SDIO";
            default:                return "CODIGO NAO MAPEADO";
        }
    }

    static void gerenciarBlinkAsync() {
        if (_blinkAtivo) {
            unsigned long atual = millis();
            if (atual - _ultimoToggle >= _intervaloBlink) {
                _ultimoToggle = atual;
                _estadoLed = !_estadoLed;
                digitalWrite(LED_PINOBLE, _estadoLed ? LOW : HIGH);
            }
        }
    }

    static void executar(const String& cmdBruto, bool requisicaoRemotaUdp = false) {
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
                   "--- DIAGNOSTICO LOCAL ---\n"
                   "help      : Lista os comandos do sistema\n"
                   "info      : Exibe status completo do mestre\n"
                   "status    : Resumo rapido de conexao e heap\n"
                   "reason    : Exibe o motivo do ultimo reset\n"
                   "version   : Versao atual do firmware mestre\n"
                   "build     : Data e hora da compilacao\n"
                   "cpu       : Modelo, cores e frequencia\n"
                   "ram       : Heap total e heap livre atual\n"
                   "flash     : Tamanho e velocidade do chip\n"
                   "temp      : Temperatura interna da CPU C\n"
                   "mac       : Endereco MAC fisico do Wi-Fi\n"
                   "net_info  : Exibe IP, RSSI e SSID atual\n"
                   "uptime    : Tempo de atividade em segundos\n"
                   "time/date : Hora e data calculadas via NTP\n"
                   "lastcmd   : Exibe a string do ultimo comando\n"
                   "reset     : Reinicia o ESP32\n"
                   "scan      : Inicia varredura de redes Wi-Fi proximas\n"
                   "--- CONFIGURACOES ---\n"
                   "set_fuso: : Altera GMT do NTP (Ex: set_fuso:-3)\n"
                   "reset_wifi: Limpa a Flash e abre o Portal AP\n"
                   "set_escravo1: [IP] : Configura IP do ESP 1\n"
                   "set_escravo2: [IP] : Configura IP do ESP 2\n"
                   "set_escravo3: [IP] : Configura IP do ESP 3\n"
                   "set_escravo4: [IP] : Configura IP do ESP 4\n"
                   "--- COMANDOS REMOTOS UDP ---\n"
                   "list      : Lista os arquivos do Cartao SD\n"
                   "read:[arq]: Le arquivo do SD (Ex: read:log.txt)\n"
                   "del:[arq] : Deleta do SD (Ex: del:log.txt)\n"
                   "========================";
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
                "Firmware: 1.0.0\n"
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
            resp = "Central Mestre:\nVersao Firmware: v1.0.0";
        }
        else if (cmd == "build") {
            resp = "Central Mestre Build:\nData: " + String(__DATE__) + "\nHora: " + String(__TIME__);
        }
        else if (cmd == "status") {
            String statusWifi = (WiFi.status() == WL_CONNECTED) ? "OK" : "FALHA";
            float heapKB = (float)ESP.getFreeHeap() / 1024;
            resp = "ONLINE (MESTRE)\nWiFi: " + statusWifi + "\nSD: N/A\nNTP: OK\nHeap: " + String(heapKB, 1) + " KB";
        }
        else if (cmd == "reset") {
            resp = "Central Local:\nReiniciando o ESP32...";
            delay(1000);
            ESP.restart();
        }
        else if (cmd == "time") {
            String horaAtual = ntp.getSomenteHora();
            resp = "Central Local:\nHora atual: " + horaAtual;
        }
        else if (cmd == "date") {
            String dataHoraCompleta; ntp.getDateTime(dataHoraCompleta, 100);
            String data = "Erro NTP";
            if (dataHoraCompleta.length() >= 19) {
                data = dataHoraCompleta.substring(0, 10);
            }
            resp = "Central Local:\nData atual: " + data;
        }
        else if (cmd == "lastcmd") {
            resp = "Mestre Ultimo Cmd:\n" + _mestreUltimoCmd;
        }
        else if (cmd == "cmdcount") {
            resp = "Mestre Cmd Count:\nTotal: " + String(_mestreTotalCmd) + " processados";
        }
        else if (cmd.startsWith("set_escravo1:")) {
            String novoIp = cmd.substring(13); novoIp.trim();
            udp.atualizarIpEscravo(1, novoIp);
            resp = "Escravo 1 IP:\n" + novoIp;
            central.renderizarTela();
        }
        else if (cmd.startsWith("set_escravo2:")) {
            String novoIp = cmd.substring(13); novoIp.trim();
            udp.atualizarIpEscravo(2, novoIp);
            resp = "Escravo 2 IP:\n" + novoIp;
            central.renderizarTela();
        }
        else if (cmd.startsWith("set_escravo3:")) {
            String novoIp = cmd.substring(13); novoIp.trim();
            udp.atualizarIpEscravo(3, novoIp);
            resp = "Escravo 3 IP:\n" + novoIp;
            central.renderizarTela();
        }
        else if (cmd.startsWith("set_escravo4:")) {
            String novoIp = cmd.substring(13); novoIp.trim();
            udp.atualizarIpEscravo(4, novoIp);
            resp = "Escravo 4 IP:\n" + novoIp;
            central.renderizarTela();
        }
        else if (cmd == "reset_wifi") {
             central.exibirTelaResposta("Limpando Flash...\nReiniciando...");
             Preferences prefs; prefs.begin("wifi", false); prefs.clear(); prefs.end();
             delay(2000); ESP.restart();
        }
        else if (cmd == "desligar") {
            central.exibirTelaResposta("Desligando Sistema...\nEntrando em modo de economia.");
            delay(2000);
            central.desligarDisplayFisico();
            esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); 
            esp_deep_sleep_start();
        }
        else if (cmd == "led_on") {
            _blinkAtivo = false; _estadoLed = true;
            digitalWrite(LED_PINOBLE, LOW); 
            resp = "Central Local:\nLED ligado com sucesso.";
        }
        else if (cmd == "led_off") {
            _blinkAtivo = false; _estadoLed = false;
            digitalWrite(LED_PINOBLE, HIGH); 
            resp = "Central Local:\nLED desligado.";
        }
        else if (cmd.startsWith("led_blink:")) {
            _intervaloBlink = cmd.substring(10).toInt();
            if (_intervaloBlink <= 0) _intervaloBlink = 500;
            _blinkAtivo = true;
            resp = "Central Local:\nBlink ativo (" + String(_intervaloBlink) + " ms)";
        }
        else if (cmd.startsWith("set_fuso:")) {
            int novoFuso = cmd.substring(9).toInt();
            if (novoFuso >= -12 && novoFuso <= 14) {
                ntp.configurarRelogio(novoFuso, false);
                resp = "Fuso alterado:\nGMT " + String(novoFuso);
            } else {
                resp = "Erro: Fuso invalido";
            }
        }
        else if (cmd == "time") {
            resp = "Hora:\n" + ntp.getSomenteHora();
        }
        else if (cmd == "date") {
            String dataHoraCompleta; ntp.getDateTime(dataHoraCompleta, 100);
            String data = (dataHoraCompleta.length() >= 10) ? dataHoraCompleta.substring(0, 10) : "Erro NTP";
            resp = "Data:\n" + data;
        }
        else if (cmd == "cpu") {
            resp = "Central Local CPU:\nModelo: " + String(ESP.getChipModel()) + "\n" +
                   "Cores: " + String(ESP.getChipCores()) + "\n" +
                   "Freq: " + String(ESP.getCpuFreqMHz()) + " MHz";
        }
        else if (cmd == "ram") {
            resp = "Central Local RAM:\nHeap Total: " + String(ESP.getHeapSize() / 1024) + " KB\n" +
                   "Heap Livre: " + String(ESP.getFreeHeap() / 1024) + " KB";
        }
        else if (cmd == "flash") {
            resp = "Central Local FLASH:\nTamanho: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB\n" +
                   "Velocidade: " + String(ESP.getFlashChipSpeed() / 1000 / 1000) + " MHz";
        }
        else if (cmd == "temp") {
            resp = "Central Local:\nCPU Temp: " + String(temperatureRead(), 2) + " C";
        }
        else if (cmd == "mac") {
            resp = "Central Local:\nMAC: " + WiFi.macAddress();
        }
        else if (cmd == "net_info") {
            resp = "Central Local NET:\nIP: " + WiFi.localIP().toString() + "\nRSSI: " + String(WiFi.RSSI()) + " dBm\nSSID: " + WiFi.SSID();
        }
        else if (cmd == "uptime") {
            resp = "Uptime:\n" + String(millis() / 1000) + " s";
        }
        else if (cmd == "scan") {
            central.imprimirRodape("Disparando Varredura...", TFT_YELLOW);
            
            // Reseta o status de todos para vermelho antes de testar
            for(int i = 1; i <= 4; i++) {
                LayoutDatabase::statusEscravos[i] = false;
            }
            central.renderizarTela();

            // Envia um ping rápido para cada um dos 4 escravos
            for (int i = 1; i <= 4; i++) {
                udp.enviarComando("alive", i); 
                delay(50); // Pequeno intervalo para não encavalar pacotes na rede
            }
            
            // Limpa o estado de espera global para não gerar falsos timeouts de botões
            udp.resetarEspera();
            
            resp = "Varredura Disparada!\nVerifique o status na tela inicial.";
            central.renderizarTela();
        }
        else 
        {
            resp = "Central Local:\nComando desconhecido.";
        }

        central.exibirTelaResposta(resp);
        if (requisicaoRemotaUdp) {
            udp.responderRemoto(resp);
        }
    }
};

#endif
