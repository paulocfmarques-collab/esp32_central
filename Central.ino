#include "Display.h"
#include "InterfaceCentral.h"
#include "WifiConfig.h"
#include "UdpComm.h"
#include "NTPUtil.h"
#include "CommandHandler.h"
#include <esp_task_wdt.h>

Display displayHardware;
InterfaceCentral central(displayHardware);
UdpComm udp(4210);
NTPUtil ntp; 

// Instância do Portal Web
WifiConfig portalWifi(displayHardware); 

// Handlers do FreeRTOS
TaskHandle_t TaskCore0;
QueueHandle_t filaMensagens;

struct PacoteRede {
  String payload;
  bool veioDeEscravo;
};

// ─── TAREFA EXECUTADA EXCLUSIVAMENTE NO CORE 0 (REDE / ESCUTA PASSIVA DE DADOS) ───
void codigoCore0(void * pvParameters) {
  Serial.print("[Core 0] Inicializado no nucleo: ");
  Serial.println(xPortGetCoreID());

  #if ESP_IDF_VERSION_MAJOR >= 5 || defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 5000,
        .idle_core_mask = (1 << 0), 
        .trigger_panic = true
    };
    esp_task_wdt_reconfigure(&twdt_config);
    esp_task_wdt_add(xTaskGetCurrentTaskHandle()); 
  #else
    esp_task_wdt_add(NULL); 
  #endif

  for(;;) {
    esp_task_wdt_reset();

    if (WiFi.status() == WL_CONNECTED) {
      OtaManager::processar(); 

      // O Core 0 funciona apenas como um escutador passivo de pacotes de rede legítimos.
      String dadosRede;
      bool veioDeEscravo = false;
      
      if (udp.escutarResposta(dadosRede, veioDeEscravo)) {
        dadosRede.trim();
        
        if (veioDeEscravo) {
          // Extrai o IP através do método público unificado da classe
          String ipRemotoReal = udp.obterIpRemotoReal();
          
          for (int i = 1; i <= 4; i++) {
            // Homologa como online o ESP cujo IP cadastrado bate com o remetente
            if (udp.obterIpEscravo(i) == ipRemotoReal) {
              bool estadoRealAnterior = LayoutDatabase::statusEscravos[i];
              LayoutDatabase::statusEscravos[i] = true; 
              
              // Só redesenha a interface inicial se o estado mudou na tela home
              if (estadoRealAnterior != true && central.obterModoOperacao() == 0) {
                central.renderizarTela();
              }
              break;
            }
          }
        }

        // Repassa pacotes textuais legítimos para a fila de comandos do Core 1
        if (dadosRede.length() >= 2) { 
          PacoteRede* novoPacote = new PacoteRede();
          novoPacote->payload = dadosRede;
          novoPacote->veioDeEscravo = veioDeEscravo;

          if (xQueueSend(filaMensagens, &novoPacote, pdMS_TO_TICKS(10)) != pdPASS) {
            Serial.println(F("[Core 0] ERRO: Fila cheia."));
            delete novoPacote;
          }
        }
      }
    }
    
    yield();
    vTaskDelay(pdMS_TO_TICKS(1)); 
  }
}

void setup() {
  Serial.begin(115200);
  
  central.otaGravando = false; 

  displayHardware.inicializar();
  central.inicializar();
  CommandHandler::inicializarHardware(); 

  filaMensagens = xQueueCreate(10, sizeof(PacoteRede*));
  if (filaMensagens == NULL) {
    Serial.println(F("Falha critica: Nao foi possivel criar a fila do FreeRTOS."));
    while(1);
  }

  // Inicialização do Portal Web e Wi-Fi
  if (portalWifi.conectar()) {
    WiFi.softAPdisconnect(true);
    
    // Desativa o Power Save do Wi-Fi para manter alta estabilidade
    WiFi.setSleep(false); 
    
    udp.inicializar();
    
    String ipLocal = WiFi.localIP().toString();
    Serial.println(F("\n-------------------------------------"));
    Serial.print(F("[WIFI] Conectado! Modo Alta Performance Ativo. IP: "));
    Serial.println(ipLocal);
    Serial.println(F("-------------------------------------"));

    String msgBoasVindas = "Central Ativa!\nIP: " + ipLocal;
    central.exibirTelaResposta(msgBoasVindas);
    delay(2000);
    
    OtaManager::inicializar(central);
    
    central.imprimirRodape("Sincronizando Relogio NTP...", TFT_YELLOW);
    ntp.initNTP(-3, false);
    
    central.renderizarTela();

    xTaskCreatePinnedToCore(
      codigoCore0,    
      "TaskCore0",    
      16384,          
      NULL,           
      5,              
      &TaskCore0,     
      0               
    );
  } 
  else {
    portalWifi.iniciarPortal();
  }
}

// ─── LOOP EXECUTADO NO CORE 1 (APLICAÇÃO / INTERFACE GRÁFICA / PORTAL WEB) ───
void loop() {
  if (OtaManager::otaSolicitado && !central.otaGravando) {
    central.otaGravando = true; 
    
    TFT_eSPI& tft = central._renderer.getDriver(); 
    tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, 320, 42, 0x2103);
    tft.setTextColor(TFT_GOLD); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
    tft.drawString("ATUALIZACAO VIA OTA", 160, 21);
    
    tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
    tft.drawString("Gravando novo firmware na memoria Flash...", 160, 80);
    tft.drawString("Nao desligue a alimentacao da Central.", 160, 100);
    tft.drawRect(20, 130, 280, 22, TFT_WHITE);
    
    Serial.println(F("[Core 1] Painel grafico travado. Liberando Core 0 para queima."));
  }

  if (central.otaGravando) {
    static int ultimaPorcentagemDesenhada = -1;
    int otaProgresso = OtaManager::porcentagemAtual;

    if (otaProgresso != ultimaPorcentagemDesenhada && otaProgresso >= 0) {
      ultimaPorcentagemDesenhada = otaProgresso;
      TFT_eSPI& tft = central._renderer.getDriver();
      
      if (otaProgresso == 100) {
          tft.fillRect(22, 132, 276, 18, TFT_GREEN); 
          tft.setTextColor(TFT_GREEN); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
          tft.drawString("GRAVACAO CONCLUIDA!", 160, 195);
          tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
          tft.drawString("Reiniciando o sistema...", 160, 215);
      } else {
          int larguraBarra = map(otaProgresso, 0, 100, 0, 276);
          tft.fillRect(22, 132, larguraBarra, 18, 0x05FF); 
          
          tft.fillRect(130, 160, 60, 15, TFT_BLACK); 
          tft.setTextColor(TFT_YELLOW); tft.setTextSize(1); tft.setTextDatum(MC_DATUM);
          tft.drawString(String(otaProgresso) + " %", 160, 168);
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); 
    return; 
  }

  // Auto-reconectador em caso de quedas físicas do sinal Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long ultimoReconect = 0;
    
    if (central.obterModoOperacao() != 0) {
       central.imprimirRodape("Link de Rede Caido! Reconectando...", TFT_RED);
    }
    
    if (millis() - ultimoReconect >= 7000) {
      ultimoReconect = millis();
      Serial.println(F("[Rede] Reiniciando soquetes por inatividade..."));
      
      WiFi.disconnect();
      WiFi.reconnect();
      udp.inicializar(); 
    }
    portalWifi.processarPortal();
    return;
  }

  // Processamento de pacotes recebidos pela fila do FreeRTOS
  PacoteRede* pacoteRecebido;
  if (xQueueReceive(filaMensagens, &pacoteRecebido, 0) == pdPASS) {
    if (pacoteRecebido->veioDeEscravo) {
      udp.resetarEspera(); 
      central.acumularMensagemResposta(pacoteRecebido->payload);
    } else {
      CommandHandler::executar(pacoteRecebido->payload, true);
    }
    delete pacoteRecebido; 
  }

  // Atualizações dinâmicas da interface gráfica e periféricos
  central.atualizarRelogioDinamico();
  central.atualizarIndicadorWifi();
  central.checarAutoFechamento();
  CommandHandler::gerenciarBlinkAsync(); 

  // Escuta ativa de comandos via Monitor Serial da Arduino IDE
  if (Serial.available() > 0) {
    String cmdSerial = Serial.readStringUntil('\n');
    if (cmdSerial.length() > 0) {
      CommandHandler::executar(cmdSerial, false);
    }
  }

  // Varredura geométrica de toques no vidro da tela TFT
  const char* comandoSolicitado = central.escanearToque();
  if (comandoSolicitado != nullptr) {
    if (central.obterModoOperacao() == 1) { 
      CommandHandler::executar(String(comandoSolicitado), false);
    } 
    else if (central.obterModoOperacao() == 2) { 
      udp.enviarComando(comandoSolicitado, central.obterEscravoAtivoAlvo()); 
    }
  }

  // Só exibe timeout de comando se estiver controlando um ESP remoto ativamente (Modo 2)
  if (central.obterModoOperacao() == 2 && udp.checarTimeout()) {
    String alvoOffline = "ESP " + String(central.obterEscravoAtivoAlvo());
    central.exibirTelaResposta("ERRO: " + alvoOffline + "\nSEM RESPOSTA (TIMEOUT)");
  } else if (central.obterModoOperacao() != 2) {
    udp.resetarEspera();
  }

  portalWifi.processarPortal();
}
