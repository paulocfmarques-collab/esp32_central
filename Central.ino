#include "Display.h"
#include "InterfaceCentral.h"
#include "WifiConfig.h"
#include "UdpComm.h"
#include "NTPUtil.h"
#include "CommandHandler.h"
#include <esp_task_wdt.h>

// Alocação de variáveis estáticas da aplicação
bool CommandHandler::_blinkAtivo = false;
bool CommandHandler::_estadoLed = false;
unsigned long CommandHandler::_ultimoToggle = 0;
unsigned long CommandHandler::_intervaloBlink = 500;

Display displayHardware;
InterfaceCentral central(displayHardware);
UdpComm udp(4210);
NTPUtil ntp; 

// Instância corrigida do Portal Web
WifiConfig portalWifi(displayHardware); 

// Handlers do FreeRTOS
TaskHandle_t TaskCore0;
QueueHandle_t filaMensagens;

struct PacoteRede {
  String payload;
  bool veioDeEscravo;
};

// ─── TAREFA EXECUTADA EXCLUSIVAMENTE NO CORE 0 (REDE / ESCUTA DE RESPOSTAS E PINGS) ───
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

      String dadosRede;
      bool veioDeEscravo = false;
      
      if (udp.escutarResposta(dadosRede, veioDeEscravo)) {
        dadosRede.trim();
        
        if (veioDeEscravo) {
          String ipRemotoReal = udp.obterIpRemotoReal();
          
          // Confere se o IP que respondeu bate com algum cadastrado
          for (int i = 1; i <= 4; i++) {
            if (udp.obterIpEscravo(i) == ipRemotoReal) {
              bool estadoRealAnterior = LayoutDatabase::statusEscravos[i];
              LayoutDatabase::statusEscravos[i] = true; // Marca como VERDE (Online)
              
              // Se a tela atual for a inicial (Modo 0), atualiza os blocos imediatamente
              if (estadoRealAnterior != true && central.obterModoOperacao() == 0) {
                central.renderizarTela();
              }
              break;
            }
          }
        }

        // Se o pacote contiver texto útil (além do ping vazio), manda para a fila do Core 1
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

  // Inicialização com as blindagens de rede aplicadas
  if (portalWifi.conectar()) {
    WiFi.softAPdisconnect(true);
    
    // ─── BLINDAGEM MÁXIMA CONTRA QUEDAS POR INATIVIDADE ───
    // Desativa o Power Save do Wi-Fi para o roteador nunca desconectar o ESP32
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

// ─── LOOP EXECUTADO NO CORE 1 (APLICAÇÃO / INTERFACE GRÁFICA) ───
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
    
    Serial.println(F("[Core 1] Painel grafico travado com seguranca. Liberando Core 0 para queima."));
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

  // ─── WATCHDOG GRÁFICO E RECONECTADOR AUTO-REGENERATIVO DE SOCKETS ───
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long ultimoReconect = 0;
    
    if (central.obterModoOperacao() != 0) {
       central.imprimirRodape("Link de Rede Caido! Reconectando...", TFT_RED);
    }
    
    if (millis() - ultimoReconect >= 7000) {
      ultimoReconect = millis();
      Serial.println(F("[Rede] Reiniciando soquetes travados por inatividade..."));
      
      WiFi.disconnect();
      WiFi.reconnect();
      udp.inicializar(); // Limpa e força uma nova tabela NAT no roteador
    }
    
    portalWifi.processarPortal();
    return;
  }

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

  central.atualizarRelogioDinamico();
  central.atualizarIndicadorWifi();
  central.checarAutoFechamento();
  CommandHandler::gerenciarBlinkAsync(); 

  if (Serial.available() > 0) {
    String cmdSerial = Serial.readStringUntil('\n');
    if (cmdSerial.length() > 0) {
      CommandHandler::executar(cmdSerial, false);
    }
  }

  const char* comandoSolicitado = central.escanearToque();
  if (comandoSolicitado != nullptr) {
    if (central.obterModoOperacao() == 1) { 
      CommandHandler::executar(String(comandoSolicitado), false);
    } 
    else if (central.obterModoOperacao() == 2) { 
      udp.enviarComando(comandoSolicitado, central.obterEscravoAtivoAlvo()); 
    }
  }

  if (central.obterModoOperacao() == 2 && udp.checarTimeout()) {
    String alvoOffline = "ESP " + String(central.obterEscravoAtivoAlvo());
    central.exibirTelaResposta("ERRO: " + alvoOffline + "\nSEM RESPOSTA (TIMEOUT)");
  } else if (central.obterModoOperacao() != 2) {
    // Garante que o estado de timeout não fique preso em background se mudarmos de tela
    udp.resetarEspera();
  }
}
