#ifndef UDP_COMM_H
#define UDP_COMM_H

#include <WiFiUdp.h>
#include <Preferences.h>

class UdpComm {
private:
  WiFiUDP _udp;
  int _porta;
  String _ipEscravo1;
  String _ipEscravo2;
  Preferences _prefs;

  unsigned long _tempoEnvio;
  bool _aguardandoResposta;
  const unsigned long TIMEOUT_MS = 4000; // Expandido para 4s para dar tempo ao SD Card

public:
  UdpComm(int porta);
  void inicializar();
  void enviarComando(const char* cmd, bool escravo1Ativo);
  bool escutarResposta(String& msgOut, bool& veioDeEscravoOut);
  bool checarTimeout();
  void resetarEspera();
  
  void atualizarIpEscravo(int numeroEscravo, const String& novoIp);
  String obterIpEscravo(int numeroEscravo) const;
  void responderRemoto(const String& msg);
};

#endif
