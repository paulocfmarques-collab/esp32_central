#include "Display.h"
#include "InterfaceCentral.h"
#include "WifiConfig.h"
#include "UdpComm.h"
#include "NTPUtil.h"
#include "CommandHandler.h"

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

  // 1. Escuta comandos via Monitor Serial local
  if (Serial.available() > 0) {
    String cmdSerial = Serial.readStringUntil('\n');
    cmdSerial.trim();
    if (cmdSerial.length() > 0) {
      CommandHandler::executar(cmdSerial, false);
    }
  }

  // 2. Processa cliques físicos na tela TFT (Isolamento Local vs Remoto CORRIGIDO)
  const char* comandoSolicitado = central.escanearToque();
  if (comandoSolicitado != nullptr) {
    // CORREÇÃO: Verifica em qual modo a tela está trabalhando no momento do toque
    if (central.obterModoOperacao() == 0) {
      // Se a tela está no modo LOCAL, processa os comandos internos do Mestre
      CommandHandler::executar(String(comandoSolicitado), false);
    } else if (central.obterModoOperacao() == 1) {
      // Se a tela está no modo REMOTO, ignora processamento local e envia para o Escravo via UDP
      udp.enviarComando(comandoSolicitado, central.isEscravo1Ativo());
    }
  }

  // 3. Escuta de mensagens UDP (Processamento Híbrido com Filtro Anti-Fantasma)
  String dadosRede;
  bool veioDeEscravo = false;
  if (udp.escutarResposta(dadosRede, veioDeEscravo)) {
    
    // Limpa espaços em branco e verifica se a mensagem é válida. 
    // Ignora pacotes vazios, mensagens com menos de 2 caracteres ou ruídos de rede.
    dadosRede.trim();
    if (dadosRede.length() >= 2) { 
      
      if (veioDeEscravo) {
        // Se veio de um escravo legítimo, desliga imediatamente o alarme de timeout
        udp.resetarEspera(); 

        // Encaminha as linhas para o terminal acumulador do display TFT
        central.acumularMensagemResposta(dadosRede);
      } else {
        #include <WiFi.h>
        // Se veio do COMPUTADOR (Script em Python). Executa no mestre e devolve o texto!
        CommandHandler::executar(dadosRede, true);
      }
      
    } else {
      // Descarta o pacote fantasma silenciosamente sem poluir o display
      Serial.println(F("[UDP] Pacote inválido ou ruído de rede descartado."));
    }
  }

  // 4. Mecanismo de Timeout para os Escravos
  if (udp.checarTimeout()) {
    String alvoOffline = central.isEscravo1Ativo() ? "ESP 1" : "ESP 2";
    central.exibirTelaResposta("ERRO: " + alvoOffline + "\nSEM RESPOSTA (TIMEOUT)");
  }
}
