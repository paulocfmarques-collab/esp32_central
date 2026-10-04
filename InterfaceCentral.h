#ifndef INTERFACE_CENTRAL_H
#define INTERFACE_CENTRAL_H

#include "TouchDriver.h"
#include "ScreenRenderer.h"
#include "LayoutDatabase.h"

class InterfaceCentral {
public:
    enum Modo { MODO_INICIAL = 0, MODO_LOCAL = 1, MODO_REMOTO = 2 };
    
    bool otaGravando;

    // Controle de temporização da varredura
    unsigned long ultimoPingCheck = 0;

    InterfaceCentral(Display& displayRef);
    void inicializar();
    void renderizarTela();
    void exibirTelaResposta(String msg);
    void acumularMensagemResposta(String msg);
    void atualizarRelogioDinamico();
    void atualizarIndicadorWifi();
    void checarAutoFechamento();
    void imprimirRodape(String msg, uint16_t cor);
    const char* escanearToque();
    void desligarDisplayFisico();

    int obterTotalPaginasDoModo() const;
    int obterModoOperacao() const { return (int)_modoOperacao; }
    int obterEscravoAtivoAlvo() const { return _escravoAtivoAlvo; }

    bool isEscravo1Ativo() const { return _escravoAtivoAlvo == 1; }
    bool isEscravo2Ativo() const { return _escravoAtivoAlvo == 2; }

    ScreenRenderer _renderer; 

private:
    TouchDriver _touch;
    Modo _modoOperacao;
    int _escravoAtivoAlvo; 
    int _paginaAtual;
    bool _modoRespostaAtivo;
    int _yTerminalDinamic;
    
    unsigned long _ultimoRelogioTS;
    unsigned long _ultimoWifiTS;
    unsigned long _tempoAberturaRespostaTS;
    String _statusAtual;
    
    unsigned long _ultimoToqueAtividadeTS; 
    bool _protecaoTelaAtiva;              
    const unsigned long TIMEOUT_PROTECAO_MS = 300000; // 5 minutos
    
    int _scrollOffsetLinhas;      // Quantas linhas rolamos para cima/baixo
    String _mensagemRespostaCompleta; // Armazena o texto inteiro para repintar no Scroll
    const int AUTO_CLOSE_MS = 30000;
    
    friend class OtaManager;
}; 

#endif
