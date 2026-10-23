#include "UdpComm.h"
#include "SdManager.h"
#include <WiFi.h>

UdpComm::UdpComm(int porta) 
  : _porta(porta), _tempoEnvio(0), _aguardandoResposta(false) {}

void UdpComm::inicializar() {
  bool aberto = _udp.begin(_porta) == 1;
  portENTER_CRITICAL(&_metricasMux);
  _metricas.listenerUp = aberto;
  portEXIT_CRITICAL(&_metricasMux);
  if (!aberto) Serial.println(F("[UDP] Falha ao abrir porta de escuta."));
  
  // Lê da Flash apenas UMA VEZ no setup e armazena na memória RAM (Cache de Proteção)
  _prefs.begin("wifi", true);
  _ipEscravo1 = _prefs.getString("ip1", "192.168.0.120"); 
  _ipEscravo2 = _prefs.getString("ip2", "192.168.0.125");
  _ipEscravo3 = _prefs.getString("ip3", "192.168.0.130"); 
  _ipEscravo4 = _prefs.getString("ip4", "192.168.0.135"); 
  _prefs.end();
}

void UdpComm::enviarComando(const char* cmd, int numeroEscravo) {
  IPAddress targetIP;
  String destinoStr = _ipEscravo1;
  
  if (numeroEscravo == 2) destinoStr = _ipEscravo2;
  else if (numeroEscravo == 3) destinoStr = _ipEscravo3;
  else if (numeroEscravo == 4) destinoStr = _ipEscravo4;
  
  if (targetIP.fromString(destinoStr.c_str())) {
    // Escoa/Limpa qualquer resíduo perdido no buffer antes de injetar uma nova transmissão
    _udp.flush(); 
    
    bool enviado = _udp.beginPacket(targetIP, _porta) == 1 &&
                   _udp.print(cmd) == strlen(cmd) && _udp.endPacket() == 1;
    registrarEnvio(enviado, false);
    SdManager::registrar(enviado ? "COMANDO_TX" : "FALHA_TX", "udp:ESP" + String(numeroEscravo) + ":" + destinoStr, cmd);
    Serial.print(F("[ESP TX "));
    Serial.print(numeroEscravo);
    Serial.print(F("] "));
    Serial.println(cmd);
    
    _tempoEnvio = millis();
    _aguardandoResposta = enviado;
  } else {
    registrarEnvio(false, false);
    SdManager::registrar("FALHA_TX", "udp:ESP" + String(numeroEscravo), "IP invalido / comando: " + String(cmd));
  }
}

bool UdpComm::escutarResposta(String& msgOut, bool& veioDeEscravoOut) {
  int packetSize = _udp.parsePacket();
  if (packetSize) {
    portENTER_CRITICAL(&_metricasMux);
    ++_metricas.pacotesRecebidos;
    portEXIT_CRITICAL(&_metricasMux);
    char packetBuffer[1460]; 
    int len = _udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = '\0';
      msgOut = String(packetBuffer);
      
      String ipRemoto = _udp.remoteIP().toString();
      
      veioDeEscravoOut = (ipRemoto == _ipEscravo1 || ipRemoto == _ipEscravo2 || 
                          ipRemoto == _ipEscravo3 || ipRemoto == _ipEscravo4);
      return true;
    }
  }
  return false;
}

bool UdpComm::checarTimeout() {
  if (_aguardandoResposta && (millis() - _tempoEnvio >= TIMEOUT_MS)) {
    _aguardandoResposta = false;
    return true;
  }
  return false;
}

void UdpComm::resetarEspera() { 
  _aguardandoResposta = false; 
}

void UdpComm::atualizarIpEscravo(int numeroEscravo, const String& novoIp) {
  IPAddress temporario;
  if (temporario.fromString(novoIp.c_str())) {
    // Atualiza a RAM imediatamente para o sistema não engasgar
    if (numeroEscravo == 1) _ipEscravo1 = novoIp;
    else if (numeroEscravo == 2) _ipEscravo2 = novoIp;
    else if (numeroEscravo == 3) _ipEscravo3 = novoIp;
    else if (numeroEscravo == 4) _ipEscravo4 = novoIp;

    // Salva na Flash em segundo plano de forma isolada
    _prefs.begin("wifi", false);
    if (numeroEscravo == 1) _prefs.putString("ip1", _ipEscravo1);
    else if (numeroEscravo == 2) _prefs.putString("ip2", _ipEscravo2);
    else if (numeroEscravo == 3) _prefs.putString("ip3", _ipEscravo3);
    else if (numeroEscravo == 4) _prefs.putString("ip4", _ipEscravo4);
    _prefs.end();
  }
}

// ✅ SEGURO E ULTRA RÁPIDO: Retorna direto da RAM sem encostar na Flash!
String UdpComm::obterIpEscravo(int numeroEscravo) const {
  if (numeroEscravo == 2) return _ipEscravo2;
  if (numeroEscravo == 3) return _ipEscravo3;
  if (numeroEscravo == 4) return _ipEscravo4;
  return _ipEscravo1;
}

void UdpComm::responderRemoto(const String& msg) {
  IPAddress destino = _udp.remoteIP();
  uint16_t porta = _udp.remotePort();
  String conteudo = msg + "\n";
  bool enviado = _udp.beginPacket(destino, porta) == 1 &&
                 _udp.print(conteudo) == conteudo.length() && _udp.endPacket() == 1;
  registrarEnvio(enviado, true);
  SdManager::registrar(enviado ? "RESPOSTA_TX" : "FALHA_TX", "udp:" + destino.toString() + ":" + String(porta), msg);
}

void UdpComm::registrarEnvio(bool sucesso, bool resposta) {
  portENTER_CRITICAL(&_metricasMux);
  if (!sucesso) ++_metricas.errosEnvio;
  else if (resposta) ++_metricas.respostasEnviadas;
  portEXIT_CRITICAL(&_metricasMux);
}

UdpComm::Metricas UdpComm::snapshotMetricas() const {
  portENTER_CRITICAL(&_metricasMux);
  Metricas copia = _metricas;
  portEXIT_CRITICAL(&_metricasMux);
  copia.listenerUp = copia.listenerUp && WiFi.status() == WL_CONNECTED;
  return copia;
}
