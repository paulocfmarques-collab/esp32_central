#include "UdpComm.h"

UdpComm::UdpComm(int porta) 
  : _porta(porta), _tempoEnvio(0), _aguardandoResposta(false) {}

void UdpComm::inicializar() {
  _udp.begin(_porta);

  _prefs.begin("wifi", true);
  _ipEscravo1 = _prefs.getString("ip1", "192.168.0.120"); 
  _ipEscravo2 = _prefs.getString("ip2", "192.168.0.125");
  _prefs.end();
}

void UdpComm::enviarComando(const char* cmd, bool escravo1Ativo) {
  IPAddress targetIP;
  String destinoStr = escravo1Ativo ? _ipEscravo1 : _ipEscravo2;
  
  if (targetIP.fromString(destinoStr.c_str())) {
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
    // Buffer expandido para aguentar blocos de leitura do Cartão SD (READ)
    char packetBuffer[1460]; 
    int len = _udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = '\0';
      msgOut = String(packetBuffer);
      
      String ipRemoto = _udp.remoteIP().toString();
      if (ipRemoto == _ipEscravo1 || ipRemoto == _ipEscravo2) {
        veioDeEscravoOut = true;
      } else {
        veioDeEscravoOut = false; 
      }
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
    _prefs.begin("wifi", false);
    if (numeroEscravo == 1) {
      _ipEscravo1 = novoIp;
      _prefs.putString("ip1", _ipEscravo1);
    } else if (numeroEscravo == 2) {
      _ipEscravo2 = novoIp;
      _prefs.putString("ip2", _ipEscravo2);
    }
    _prefs.end();
  }
}

String UdpComm::obterIpEscravo(int numeroEscravo) const {
  return (numeroEscravo == 1) ? _ipEscravo1 : _ipEscravo2;
}

void UdpComm::responderRemoto(const String& msg) {
  _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
  _udp.print(msg + "\n");
  _udp.endPacket();
}
