#include "Display.h"
#include "InterfaceCentral.h"
#include "WifiConfig.h"
#include "UdpComm.h"
#include "NTPUtil.h"
#include "CommandHandler.h"

// ─── ALOCAÇÃO DE VARIÁVEIS ESTÁTICAS DE HARDWARE DO MESTRE ───
bool CommandHandler::_blinkAtivo = false;
bool CommandHandler::_estadoLed = false;
unsigned long CommandHandler::_ultimoToggle = 0;
unsigned long CommandHandler::_intervaloBlink = 500;

Display displayHardware;
InterfaceCentral central(displayHardware);
WifiConfig wifi(displayHardware);
UdpComm udp(4210);
NTPUtil ntp; 

void setup() {
  Serial.begin(115200);
  
  displayHardware.inicializar();
  central.inicializar();
  CommandHandler::inicializarHardware(); 

  if (wifi.conectar()) {
    WiFi.softAPdisconnect(true);
    udp.inicializar();
    
    String ipLocal = WiFi.localIP().toString();
    Serial.println(F("\n-------------------------------------"));
    Serial.print(F("[WIFI] Conectado! IP da Central: "));
    Serial.println(ipLocal);
    Serial.println(F("-------------------------------------"));

    String msgBoasVindas = "Central Ativa!\nIP: " + ipLocal;
    central.exibirTelaResposta(msgBoasVindas);
    delay(2000);
    
    OtaManager::inicializar(central);
    
    central.imprimirRodape("Sincronizando Relogio NTP...", TFT_YELLOW);
    ntp.initNTP(-3, false);
    
    central.renderizarTela();
  } else {
    wifi.iniciarPortal();
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    wifi.processarPortal();
    return;
  }

  OtaManager::processar();
  central.atualizarRelogioDinamico();
  central.atualizarIndicadorWifi();
  central.checarAutoFechamento();
  CommandHandler::gerenciarBlinkAsync(); 

  if (Serial.available() > 0) {
    String cmdSerial = Serial.readStringUntil('\n');
    cmdSerial.trim();
    if (cmdSerial.length() > 0) {
      CommandHandler::executar(cmdSerial, false);
    }
  }

  const char* comandoSolicitado = central.escanearToque();
  if (comandoSolicitado != nullptr) {
    if (central.obterModoOperacao() == 0) {
      CommandHandler::executar(String(comandoSolicitado), false);
    } else if (central.obterModoOperacao() == 1) {
      udp.enviarComando(comandoSolicitado, central.isEscravo1Ativo());
    }
  }

  String dadosRede;
  bool veioDeEscravo = false;
  if (udp.escutarResposta(dadosRede, veioDeEscravo)) {
    dadosRede.trim();
    if (dadosRede.length() >= 2) { 
      if (veioDeEscravo) {
        udp.resetarEspera(); 
        central.acumularMensagemResposta(dadosRede);
      } else {
        CommandHandler::executar(dadosRede, true);
      }
    }
  }

  if (udp.checarTimeout()) {
    String alvoOffline = central.isEscravo1Ativo() ? "ESP 1" : "ESP 2";
    central.exibirTelaResposta("ERRO: " + alvoOffline + "\nSEM RESPOSTA (TIMEOUT)");
  }
}
