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

class OtaManager {
public:
    // Variáveis inline C++17 impedem o erro de 'multiple definition' entre os arquivos .cpp
    inline static volatile int porcentagemAtual = -1;
    inline static volatile bool otaSolicitado = false;

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

class CommandHandler {
private:
    static bool _blinkAtivo;
    static bool _estadoLed;
    static unsigned long _ultimoToggle;
    static unsigned long _intervaloBlink;
    static String _mestreUltimoCmd;
    static uint32_t _mestreTotalCmd;

public:
    
    static String obterVersaoAutomatica() {
        // Extração matemática da Data (AAMMDD)
        int ano = ((__DATE__[9] - '0') * 10) + (__DATE__[10] - '0');
        
        int mes = (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n') ? 1 :
                  (__DATE__[0] == 'F')                                             ? 2 :
                  (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r') ? 3 :
                  (__DATE__[0] == 'A' && __DATE__[1] == 'p')                       ? 4 :
                  (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y') ? 5 :
                  (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n') ? 6 :
                  (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l') ? 7 :
                  (__DATE__[0] == 'A' && __DATE__[1] == 'u')                       ? 8 :
                  (__DATE__[0] == 'S')                                             ? 9 :
                  (__DATE__[0] == 'O')                                             ? 10 :
                  (__DATE__[0] == 'N')                                             ? 11 :
                  (__DATE__[0] == 'D')                                             ? 12 : 0;
                  
        int dia = (__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 + (__DATE__[5] - '0');

        // Extração matemática do Horário (HHMM)
        int hora   = ((__TIME__[0] - '0') * 10) + (__TIME__[1] - '0');
        int minuto = ((__TIME__[3] - '0') * 10) + (__TIME__[4] - '0');

        // Monta a string de forma segura usando buffers de formatação estáveis
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%02d%02d%02d.%02d.%02d", ano, mes, dia, hora, minuto);
        
        return String(buffer);
    }

    static void inicializarHardware();
    static String obterMotivoReset();
    static void gerenciarBlinkAsync();
    static void executar(const String& cmdBruto, bool requisicaoRemotaUdp = false);
};

#endif
