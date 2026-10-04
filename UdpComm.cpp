#include "UdpComm.h"

UdpComm::UdpComm(int porta) 
  : _porta(porta), _tempoEnvio(0), _aguardandoResposta(false) {}

void UdpComm::inicializar() {
  _udp.begin(_porta);
  
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
    
    _udp.beginPacket(targetIP, _porta);
    _udp.print(cmd);
    _udp.endPacket();
    
    _tempoEnvio = millis();
    _aguardandoResposta = true;
  }
}

bool UdpComm::escutarResposta(String& msgOut, bool& veioDeEscravoOut) {
  int packetSize = _udp.parsePacket();
  if (packetSize) {
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
  _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
  _udp.print(msg + "\n");
  _udp.endPacket();
}
