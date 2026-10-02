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

static String _mestreUltimoCmd = "Nenhum";
static uint32_t _mestreTotalCmd = 0;

class OtaManager {
public:
    static void inicializar(InterfaceCentral& IC) {
        ArduinoOTA.setHostname("ESP32_CENTRAL_MESTRE");
        ArduinoOTA.onStart([&IC]() {
            String tipo = (ArduinoOTA.getCommand() == U_FLASH) ? "Sketch" : "Filesystem";
            IC.exibirTelaResposta("OTA ATIVO!\nGravando novo " + tipo + "\nAguarde...");
        });
        ArduinoOTA.onEnd([&IC]() {
            IC.exibirTelaResposta("OTA CONCLUIDO!\nReiniciando a Central...");
            delay(1000);
        });
        ArduinoOTA.onProgress([&IC](unsigned int progresso, unsigned int total) {
            int porcentagem = progresso / (total / 100);
            String barra = "Progresso: " + String(porcentagem) + "%\n[";
            int blocks = porcentagem / 5;
            for(int i = 0; i < 20; i++) barra += (i < blocks) ? "=" : " ";
            barra += "]";
            IC.exibirTelaResposta("ATUALIZANDO OTA\n" + barra);
        });
        ArduinoOTA.onError([&IC](ota_error_t erro) {
            IC.exibirTelaResposta("ERRO NO OTA!");
            delay(3000);
            IC.renderizarTela();
        });
        ArduinoOTA.begin();
    }
    static void processar() { ArduinoOTA.handle(); }
};

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

    static void executar(const String& cmd, bool requisicaoRemotaUdp = false) {
        if (cmd != "LASTCMD" && cmd != "CMDCOUNT") {
            _mestreUltimoCmd = cmd;
            _mestreTotalCmd++;
        }

        Serial.print(F("Comando Mestre processando: "));
        Serial.println(cmd);
        String resp = "";

        if (cmd == "INFO") {
            String dataHoraCompleta; ntp.getDateTime(dataHoraCompleta, 100);
            String dataStr = "Sem Sinc.", horaStr = "--:--:--";
            if (dataHoraCompleta.length() >= 19) {
                dataStr = dataHoraCompleta.substring(0, 10);
                horaStr = dataHoraCompleta.substring(11, 19);
            }
            uint32_t heapLivre = ESP.getFreeHeap() / 1024;
            float flashLivre = (float)ESP.getFreeSketchSpace() / (1024.0 * 1024.0);

            resp = "===== DEVICE INFO =====\n"
                "Hostname: ESP32_MESTRE\n"
                "Firmware: 1.0.0_MSTR\n"
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
                "=======================";
        }
        else if (cmd == "VERSION") {
            resp = "Central Mestre:\nVersao Firmware: v1.0.0";
        }
        else if (cmd == "BUILD") {
            resp = "Central Mestre Build:\nData: " + String(__DATE__) + "\nHora: " + String(__TIME__);
        }
        else if (cmd == "STATUS") {
            String statusWifi = (WiFi.status() == WL_CONNECTED) ? "OK" : "FALHA";
            uint32_t heapKB = ESP.getFreeHeap() / 1024;
            resp = "ONLINE (MESTRE)\nWiFi: " + statusWifi + "\nSD: N/A\nNTP: OK\nHeap: " + String(heapKB) + " KB";
        }
        else if (cmd == "LASTCMD") {
            resp = "Mestre Ultimo Cmd:\n" + _mestreUltimoCmd;
        }
        else if (cmd == "CMDCOUNT") {
            resp = "Mestre Cmd Count:\nTotal: " + String(_mestreTotalCmd) + " processados";
        }
        else if (cmd.startsWith("SET_ESCRAVO1:")) {
            String novoIp = cmd.substring(13); novoIp.trim();
            udp.atualizarIpEscravo(1, novoIp);
            resp = "Escravo 1 IP:\n" + novoIp;
            central.renderizarTela();
        }
        else if (cmd.startsWith("SET_ESCRAVO2:")) {
            String novoIp = cmd.substring(13); novoIp.trim();
            udp.atualizarIpEscravo(2, novoIp);
            resp = "Escravo 2 IP:\n" + novoIp;
            central.renderizarTela();
        }
        else if (cmd == "RESET_WIFI") {
             central.exibirTelaResposta("Limpando Flash...\nReiniciando...");
             Preferences prefs; prefs.begin("wifi", false); prefs.clear(); prefs.end();
             delay(2000); ESP.restart();
        }
        else if (cmd == "DESLIGAR") {
            central.exibirTelaResposta("Desligando Sistema...\nEntrando em modo de economia.");
            delay(2000);
            central.desligarDisplayFisico();
            esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); 
            esp_deep_sleep_start();
        }
        else if (cmd == "LED_ON") {
            _blinkAtivo = false; _estadoLed = true;
            digitalWrite(LED_PINOBLE, LOW); 
            resp = "Central Local:\nLED ligado com sucesso.";
        }
        else if (cmd == "LED_OFF") {
            _blinkAtivo = false; _estadoLed = false;
            digitalWrite(LED_PINOBLE, HIGH); 
            resp = "Central Local:\nLED desligado.";
        }
        else if (cmd.startsWith("LED_BLINK:")) {
            _intervaloBlink = cmd.substring(10).toInt();
            if (_intervaloBlink <= 0) _intervaloBlink = 500;
            _blinkAtivo = true;
            resp = "Central Local:\nBlink ativo (" + String(_intervaloBlink) + " ms)";
        }
        else if (cmd.startsWith("SET_FUSO:")) {
            int novoFuso = cmd.substring(9).toInt();
            if (novoFuso >= -12 && novoFuso <= 14) {
                ntp.configurarRelogio(novoFuso, false);
                resp = "Fuso alterado:\nGMT " + String(novoFuso);
            } else {
                resp = "Erro: Fuso invalido";
            }
        }
        else if (cmd == "TIME") {
            resp = "Hora:\n" + ntp.getSomenteHora();
        }
        else if (cmd == "DATE") {
            String dataHoraCompleta; ntp.getDateTime(dataHoraCompleta, 100);
            String data = (dataHoraCompleta.length() >= 10) ? dataHoraCompleta.substring(0, 10) : "Erro NTP";
            resp = "Data:\n" + data;
        }
        else if (cmd == "CPU") {
            resp = "Central Local CPU:\nModelo: " + String(ESP.getChipModel()) + "\n" +
                   "Cores: " + String(ESP.getChipCores()) + "\n" +
                   "Freq: " + String(ESP.getCpuFreqMHz()) + " MHz";
        }
        else if (cmd == "RAM") {
            resp = "Central Local RAM:\nHeap Total: " + String(ESP.getHeapSize()) + " B\n" +
                   "Heap Livre: " + String(ESP.getFreeHeap()) + " B";
        }
        else if (cmd == "FLASH") {
            resp = "Central Local FLASH:\nTamanho: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB\n" +
                   "Velocidade: " + String(ESP.getFlashChipSpeed() / 1000 / 1000) + " MHz";
        }
        else if (cmd == "TEMP") {
            resp = "Central Local:\nCPU Temp: " + String(temperatureRead(), 2) + " C";
        }
        else if (cmd == "MAC") {
            resp = "Central Local:\nMAC: " + WiFi.macAddress();
        }
        else if (cmd == "NET_INFO") {
            resp = "Central Local NET:\nIP: " + WiFi.localIP().toString() + "\nRSSI: " + String(WiFi.RSSI()) + " dBm\nSSID: " + WiFi.SSID();
        }
        else if (cmd == "UPTIME") {
            resp = "Uptime:\n" + String(millis() / 1000) + " s";
        }
        else {
            resp = "Central Local:\nComando desconhecido.";
        }

        central.exibirTelaResposta(resp);
        if (requisicaoRemotaUdp) {
            udp.responderRemoto(resp);
        }
    }
};

#endif
