#ifndef UDP_COMM_H
#define UDP_COMM_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <Preferences.h>

class UdpComm {
public:
    UdpComm(int porta);
    void inicializar();
    void enviarComando(const char* cmd, int numeroEscravo);
    bool escutarResposta(String& msgOut, bool& veioDeEscravoOut);
    bool checarTimeout();
    void resetarEspera();
    void atualizarIpEscravo(int numeroEscravo, const String& novoIp);
    String obterIpEscravo(int numeroEscravo) const;
    void responderRemoto(const String& msg);
    String obterIpRemotoReal() { return _udp.remoteIP().toString(); }

private:
    WiFiUDP _udp;
    Preferences _prefs;
    int _porta;
    unsigned long _tempoEnvio;
    bool _aguardandoResposta;
    
    // Variáveis de cache na RAM para blindar a Flash contra acessos concorrentes rápidos
    String _ipEscravo1;
    String _ipEscravo2;
    String _ipEscravo3;
    String _ipEscravo4;
    
    const unsigned long TIMEOUT_MS = 2500; // Alinhado com a taxa de atualização
};

#endif
