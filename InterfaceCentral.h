#ifndef INTERFACE_CENTRAL_H
#define INTERFACE_CENTRAL_H

#include <XPT2046_Touchscreen.h>
#include "Display.h"
#include "Botao.h"

class InterfaceCentral {
private:
  Display& _display;
  SPIClass _touchSPI;
  XPT2046_Touchscreen _ts;
  
  bool _escravo1Ativo;
  unsigned long _ultimoTouch;
  unsigned long _ultimoRelogio;
  unsigned long _ultimoWifiCheck;  
  unsigned long _tempoAberturaResposta; 
  
  String _statusAtual;
  int _paginaAtual;
  
  int _modoOperacao; 
  static const int MODO_INICIAL = -1;
  static const int MODO_LOCAL = 0;
  static const int MODO_REMOTO = 1;

  bool _modoRespostaAtivo;
  String _mensagemResposta;
  const unsigned long AUTO_CLOSE_MS = 7000; 

  // CORRIGIDO: Total real e absoluto de inicializadores na lista
  static const int TOTAL_BOTOES = 24; 
  Botao _botoesAcao[TOTAL_BOTOES];
  
  const int btnLocalX = 20,  btnLocalY = 110, btnLocalW = 130, btnLocalH = 60;
  const int btnRemotoX = 170, btnRemotoY = 110, btnRemotoW = 130, btnRemotoH = 60;

  const int sel1X = 15,  sel1Y = 48, sel1W = 135, sel1H = 35;
  const int sel2X = 170, sel2Y = 48, sel2W = 135, sel2H = 35;
  const int btnProxX = 15, btnProxY = 185, btnProxW = 290, btnProxH = 32;

  int obterTotalPaginasDoModo() const;

public:
  InterfaceCentral(Display& displayRef);
  void inicializar();
  bool isEscravo1Ativo() const;
  void renderizarTela();
  void exibirTelaResposta(String msg);
  void acumularMensagemResposta(String msg); 
  void atualizarRelogioDinamico();
  void atualizarIndicadorWifi();   
  void checarAutoFechamento();     
  void imprimirRodape(String msg, uint16_t cor);
  const char* escanearToque();
  int obterModoOperacao() const { return _modoOperacao; }
  void desligarDisplayFisico();

};

#endif
