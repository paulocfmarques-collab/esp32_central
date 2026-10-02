#include "InterfaceCentral.h"
#include "NTPUtil.h"
#include <WiFi.h>

static int yTerminal = 60; 

InterfaceCentral::InterfaceCentral(Display& displayRef) 
  : _display(displayRef), _touchSPI(HSPI), _ts(XPT_CS), 
    _escravo1Ativo(true), _ultimoTouch(0), _ultimoRelogio(0), _ultimoWifiCheck(0), _tempoAberturaResposta(0),
    _statusAtual("Selecione o modo de operacao..."), _paginaAtual(0),
    _modoOperacao(MODO_INICIAL), 
    _modoRespostaAtivo(false), _mensagemResposta(""),
    _botoesAcao{
      // ════════════════ MODO LOCAL (Central Mestre) ════════════════
      // MODO LOCAL - PAGINA 1 (Índice 0)
      {10,  95,  94, 26, "M_LED ON",   "LED_ON",        TFT_GREEN,  0}, 
      {113, 95,  94, 26, "M_LED OFF",  "LED_OFF",       TFT_RED,    0},
      {216, 95,  94, 26, "M_BLINK 1s", "LED_BLINK:1000",TFT_PURPLE, 0},
      {10,  130, 94, 26, "M_TEMP",     "TEMP",          TFT_BLUE,   0},
      {113, 130, 94, 26, "M_CPU",      "CPU",           TFT_ORANGE, 0},
      {216, 130, 94, 26, "M_RAM",      "RAM",           0x51D0,     0},
      // MODO LOCAL - PAGINA 2 (Índice 1)
      {10,  95,  94, 26, "M_FLASH",    "FLASH",         0x91a4,     1}, 
      {113, 95,  94, 26, "M_INIT",     "INIT",          0x7BEF,     1},
      {216, 95,  94, 26, "M_UPTIME",   "UPTIME",        0x03E0,     1},
      {10,  130, 94, 26, "M_MAC",      "MAC",           0xB1DF,     1},
      {113, 130, 94, 26, "M_NET",      "NET_INFO",      0x05FF,     1},
      {216, 130, 94, 26, "RST WIFI",   "RESET_WIFI",    0xA000,     1},

      // ════════════════ MODO REMOTO (Controle UDP) ════════════════
      // MODO REMOTO - PAGINA 1 (Índice 0)
      {10,  95,  94, 26, "R_LED ON",   "LED_ON",        TFT_GREEN,  0}, 
      {113, 95,  94, 26, "R_LED OFF",  "LED_OFF",       TFT_RED,    0},
      {216, 95,  94, 26, "R_BLINK 1s", "LED_BLINK:1000",TFT_PURPLE, 0},
      {10,  130, 94, 26, "R_TEMP",     "TEMP",          TFT_BLUE,   0},
      {113, 130, 94, 26, "R_CPU",      "CPU",           TFT_ORANGE, 0},
      {216, 130, 94, 26, "R_RAM",      "RAM",           0x51D0,     0},
      // MODO REMOTO - PAGINA 2 (Índice 1)
      {10,  95,  94, 26, "SD LIST",    "LIST",          TFT_BLUE,   1}, 
      {113, 95,  94, 26, "SD READ",    "READ:log.txt",  TFT_NAVY,   1},
      {216, 95,  94, 26, "SD DEL",     "DEL:log.txt",   0x91a4,     1},
      {10,  130, 94, 26, "R_PISCA",    "LED_PISCA:5:200",TFT_ORANGE,1},
      {113, 130, 94, 26, "R_UPTIME",   "UPTIME",        0x03E0,     1},
      {216, 130, 94, 26, "R_NET",      "NET_INFO",      TFT_MAROON, 1}
    }
{}

void InterfaceCentral::inicializar() {
  _touchSPI.begin(XPT_CLK, XPT_MISO, XPT_MOSI, XPT_CS);
  _ts.begin(_touchSPI); 
  _ts.setRotation(1);
}

bool InterfaceCentral::isEscravo1Ativo() const { 
  return _escravo1Ativo; 
}

int InterfaceCentral::obterTotalPaginasDoModo() const {
  if (_modoOperacao == MODO_LOCAL) return 2;  
  if (_modoOperacao == MODO_REMOTO) return 2; 
  return 0;
}

void InterfaceCentral::renderizarTela() {
  _modoRespostaAtivo = false;
  yTerminal = 60; 
  TFT_eSPI& tft = _display.getTftDriver();
  tft.fillScreen(TFT_BLACK);
  
  tft.fillRect(0, 0, SCREEN_W, 42, 0x2103);
  tft.setTextColor(TFT_GOLD); tft.setTextSize(2); tft.setTextDatum(TL_DATUM);
  tft.drawString(" CENTRAL UDP", 10, 12);
  
  _ultimoRelogio = 0;
  _ultimoWifiCheck = 0;
  atualizarRelogioDinamico();
  atualizarIndicadorWifi();

  if (_modoOperacao == MODO_INICIAL) {
    tft.setTextSize(1); tft.setTextDatum(MC_DATUM);
    
    tft.fillRoundRect(btnLocalX, btnLocalY, btnLocalW, btnLocalH, 6, TFT_BLUE);
    tft.drawRoundRect(btnLocalX, btnLocalY, btnLocalW, btnLocalH, 6, TFT_WHITE);
    tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
    tft.drawString("LOCAL", btnLocalX + (btnLocalW / 2), btnLocalY + (btnLocalH / 2));

    tft.fillRoundRect(btnRemotoX, btnRemotoY, btnRemotoW, btnRemotoH, 6, 0x03E0); 
    tft.drawRoundRect(btnRemotoX, btnRemotoY, btnRemotoW, btnRemotoH, 6, TFT_WHITE);
    tft.drawString("REMOTO", btnRemotoX + (btnRemotoW / 2), btnRemotoY + (btnRemotoH / 2));
    
    imprimirRodape("Escolha o modo de operacao para continuar.", TFT_LIGHTGREY);
    return;
  }

  if (_modoOperacao == MODO_REMOTO) {
    tft.setTextSize(1); tft.setTextDatum(MC_DATUM);
    tft.fillRoundRect(sel1X, 52, sel1W, sel1H, 4, _escravo1Ativo ? 0x0410 : TFT_DARKGREY);
    tft.drawRoundRect(sel1X, 52, sel1W, sel1H, 4, _escravo1Ativo ? TFT_CYAN : TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("ESP 1 (120)", sel1X + (sel1W / 2), 52 + (sel1H / 2));

    tft.fillRoundRect(sel2X, 52, sel2W, sel2H, 4, !_escravo1Ativo ? 0x0410 : TFT_DARKGREY);
    tft.drawRoundRect(sel2X, 52, sel2W, sel2H, 4, !_escravo1Ativo ? TFT_CYAN : TFT_WHITE);
    tft.drawString("ESP 2 (125)", sel2X + (sel2W / 2), 52 + (sel2H / 2));
  } else {
    tft.fillRect(15, 52, 290, 35, 0x10A2);
    tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.setTextDatum(MC_DATUM);
    tft.drawString("PAINEL DE COMANDOS INTERNOS DO MESTRE", 160, 69);
  }

  tft.setTextSize(1);
  for (int i = 0; i < TOTAL_BOTOES; i++) {
    bool pertenceAoModo = (_modoOperacao == MODO_LOCAL && i < 12) || (_modoOperacao == MODO_REMOTO && i >= 12);
    
    if (pertenceAoModo && _botoesAcao[i].pagina == _paginaAtual) {
      tft.fillRoundRect(_botoesAcao[i].x, _botoesAcao[i].y, _botoesAcao[i].w, _botoesAcao[i].h, 3, _botoesAcao[i].cor);
      tft.drawRoundRect(_botoesAcao[i].x, _botoesAcao[i].y, _botoesAcao[i].w, _botoesAcao[i].h, 3, TFT_WHITE);
      
      if (_botoesAcao[i].cor == TFT_GREEN || _botoesAcao[i].cor == TFT_ORANGE || _botoesAcao[i].cor == TFT_YELLOW) {
        tft.setTextColor(TFT_BLACK);
      } else {
        tft.setTextColor(TFT_WHITE);
      }
      tft.drawString(_botoesAcao[i].label, _botoesAcao[i].x + (_botoesAcao[i].w / 2), _botoesAcao[i].y + (_botoesAcao[i].h / 2));
    }
  }

  int maxPaginas = obterTotalPaginasDoModo();
  tft.fillRoundRect(btnProxX, btnProxY, btnProxW, btnProxH, 4, 0x3186);
  tft.drawRoundRect(btnProxX, btnProxY, btnProxW, btnProxH, 4, TFT_WHITE);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
  
  if (_paginaAtual == (maxPaginas - 1)) {
    tft.fillRoundRect(btnProxX, btnProxY, btnProxW, btnProxH, 4, TFT_RED); 
    tft.drawRoundRect(btnProxX, btnProxY, btnProxW, btnProxH, 4, TFT_WHITE);
    tft.drawString("VOLTAR AO MENU INICIAL", btnProxX + (btnProxW / 2), btnProxY + (btnProxH / 2));
  } else {
    String nomeModo = (_modoOperacao == MODO_LOCAL) ? "LOCAIS" : "REMOTOS";
    tft.drawString("PROXIMOS COMANDOS " + nomeModo + " (PAG " + String(_paginaAtual + 2) + "/" + String(maxPaginas) + ")", btnProxX + (btnProxW / 2), btnProxY + (btnProxH / 2));
  }
  
  imprimirRodape(_statusAtual, TFT_LIGHTGREY);
}

void InterfaceCentral::exibirTelaResposta(String msg) {
  _modoRespostaAtivo = true;
  _mensagemResposta = msg;
  _tempoAberturaResposta = millis();
  
  TFT_eSPI& tft = _display.getTftDriver();
  tft.fillRect(0, 43, SCREEN_W, SCREEN_H - 43, TFT_BLACK);
  
  // Alinhamento à Esquerda e no Topo (TL_DATUM) para simular um terminal profissional
  tft.setTextColor(TFT_GREEN); 
  tft.setTextSize(1); 
  tft.setTextDatum(TL_DATUM);

  int yInicial = 52;        // Começa logo abaixo do cabeçalho
  int espacamentoLinha = 14; // Altura segura para fonte tamanho 1
  int xMargemEsquerda = 10;  // Margem de respiro na borda esquerda
  int larguraMaximaJanela = SCREEN_W - 20; // 300 pixels utilizáveis

  int indiceAtual = 0;
  while (indiceAtual < msg.length()) {
    int proximaQuebra = msg.indexOf('\n', indiceAtual);
    String paragrafoToken;
    
    if (proximaQuebra == -1) {
      paragrafoToken = msg.substring(indiceAtual);
      indiceAtual = msg.length();
    } else {
      paragrafoToken = msg.substring(indiceAtual, proximaQuebra);
      indiceAtual = proximaQuebra + 1;
    }
    
    paragrafoToken.trim();
    if (paragrafoToken.length() == 0) continue;

    // ─── ALGORITMO INTELIGENTE DE QUEBRA DE LINHA (WORD WRAP) ───
    String linhaAcumulada = "";
    int inicioPalavra = 0;

    while (inicioPalavra < paragrafoToken.length()) {
      int proximoEspaco = paragrafoToken.indexOf(' ', inicioPalavra);
      String palavra;
      
      if (proximoEspaco == -1) {
        palavra = paragrafoToken.substring(inicioPalavra);
        inicioPalavra = paragrafoToken.length();
      } else {
        palavra = paragrafoToken.substring(inicioPalavra, proximoEspaco + 1);
        inicioPalavra = proximoEspaco + 1;
      }

      // Testaria o tamanho se adicionasse essa palavra
      String testeLinha = linhaAcumulada + palavra;
      if (tft.textWidth(testeLinha) > larguraMaximaJanela) {
        // Se estourar a largura da tela, desenha o que já tinha e pula a linha vertical
        tft.drawString(linhaAcumulada, xMargemEsquerda, yInicial);
        yInicial += espacamentoLinha;
        linhaAcumulada = palavra; // A palavra que estourou inicia a próxima linha
      } else {
        linhaAcumulada = testeLinha;
      }
    }
    
    // Desenha o restante da linha final do parágrafo
    if (linhaAcumulada.length() > 0) {
      tft.drawString(linhaAcumulada, xMargemEsquerda, yInicial);
      yInicial += espacamentoLinha;
    }

    if (yInicial > 200) break; // Evita invadir o espaço do rodapé
  }
  
  tft.setTextColor(TFT_DARKGREY); 
  tft.setTextSize(1); 
  tft.setTextDatum(BC_DATUM);
  tft.drawString("[ Toque ou aguarde 7s para voltar ao menu ]", SCREEN_W / 2, SCREEN_H - 15);
}

void InterfaceCentral::acumularMensagemResposta(String msg) {
  if (!_modoRespostaAtivo) {
    exibirTelaResposta(msg);
    return;
  }
  
  _tempoAberturaResposta = millis();
  TFT_eSPI& tft = _display.getTftDriver();
  
  tft.setTextColor(TFT_GREEN); 
  tft.setTextSize(1); 
  tft.setTextDatum(TL_DATUM);

  int indiceAtual = 0;
  while (indiceAtual < msg.length()) {
    int proximaQuebra = msg.indexOf('\n', indiceAtual);
    String linhaToken;
    
    if (proximaQuebra == -1) {
      linhaToken = msg.substring(indiceAtual);
      indiceAtual = msg.length();
    } else {
      linhaToken = msg.substring(indiceAtual, proximaQuebra);
      indiceAtual = proximaQuebra + 1;
    }
    
    if (linhaToken.length() > 0) {
      if (yTerminal > 205) {
        tft.fillRect(0, 43, SCREEN_W, SCREEN_H - 43 - 18, TFT_BLACK);
        yTerminal = 60;
      }
      tft.drawString(linhaToken, 15, yTerminal);
      yTerminal += 13;
    }
  }
}

void InterfaceCentral::atualizarRelogioDinamico() {
  unsigned long agora = millis();
  if (agora - _ultimoRelogio >= 1000) {
    _ultimoRelogio = agora;
    TFT_eSPI& tft = _display.getTftDriver();
    tft.fillRect(230, 24, 85, 14, 0x2103);
    tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.setTextDatum(TR_DATUM);
    tft.drawString(ntp.getSomenteHora(), 310, 24);
  }
}

void InterfaceCentral::atualizarIndicadorWifi() {
  unsigned long agora = millis();
  if (agora - _ultimoWifiCheck >= 5000 || _ultimoWifiCheck == 0) {
    _ultimoWifiCheck = agora;
    TFT_eSPI& tft = _display.getTftDriver();
    
    long rssi = WiFi.RSSI();
    uint16_t corSinal = TFT_GREEN;
    int barrasAtivas = 4;
    
    if (rssi < -85) { corSinal = TFT_RED; barrasAtivas = 1; }
    else if (rssi < -75) { corSinal = TFT_YELLOW; barrasAtivas = 2; }
    else if (rssi < -65) { corSinal = TFT_GREEN; barrasAtivas = 3; }
    else { corSinal = TFT_GREEN; barrasAtivas = 4; }

    tft.fillRect(200, 6, 115, 16, 0x2103);
    
    tft.setTextSize(1);
    String wifiStr = String(rssi) + "dBm";
    int larguraTexto = tft.textWidth(wifiStr);

    tft.setTextColor(corSinal); tft.setTextDatum(TR_DATUM);
    tft.drawString(wifiStr, 310, 8);

    int baseBarraX = 310 - larguraTexto - 22; 
    int baseBarraY = 18; 

    for (int b = 1; b <= 4; b++) {
      int alturaBarra = b * 3;          
      int xPos = baseBarraX + (b * 4);   
      int yPos = baseBarraY - alturaBarra;
      uint16_t corBarra = (b <= barrasAtivas) ? corSinal : TFT_DARKGREY;
      tft.fillRect(xPos, yPos, 2, alturaBarra, corBarra);
    }
  }
}

void InterfaceCentral::checarAutoFechamento() {
  if (_modoRespostaAtivo && (millis() - _tempoAberturaResposta >= AUTO_CLOSE_MS)) {
    renderizarTela();
  }
}

void InterfaceCentral::imprimirRodape(String msg, uint16_t cor) {
  _statusAtual = msg;
  if (_statusAtual.length() > 45) {
    _statusAtual = _statusAtual.substring(0, 42) + "...";
  }
  TFT_eSPI& tft = _display.getTftDriver();
  tft.fillRect(0, 222, SCREEN_W, 18, 0x1082);
  tft.setTextColor(cor); tft.setTextSize(1); tft.setTextDatum(MC_DATUM);
  tft.drawString(_statusAtual, SCREEN_W / 2, 231);
}

const char* InterfaceCentral::escanearToque() {
  if (!_ts.touched()) return nullptr;

  TS_Point p = _ts.getPoint();
  unsigned long agora = millis();

  if (!(p.z > 150 && p.x > 0 && (agora - _ultimoTouch > 350))) return nullptr;
  _ultimoTouch = agora;

  if (_modoRespostaAtivo) {
    renderizarTela();
    return nullptr;
  }

  int x = map(p.x, 300, 3900, 0, SCREEN_W);
  int y = map(p.y, 200, 3700, 0, SCREEN_H);

  if (_modoOperacao == MODO_INICIAL) {
    if (x >= btnLocalX && x <= (btnLocalX + btnLocalW) && y >= btnLocalY && y <= (btnLocalY + btnLocalH)) {
      _modoOperacao = MODO_LOCAL; _paginaAtual = 0; 
      _statusAtual = "Menu Local - Pagina 1"; renderizarTela();
    }
    else if (x >= btnRemotoX && x <= (btnRemotoX + btnRemotoW) && y >= btnRemotoY && y <= (btnRemotoY + btnRemotoH)) {
      _modoOperacao = MODO_REMOTO; _paginaAtual = 0; 
      _statusAtual = "Menu Remoto - Pagina 1"; renderizarTela();
    }
    return nullptr;
  }

  if (_modoOperacao == MODO_REMOTO) {
    if (x >= sel1X && x <= (sel1X + sel1W) && y >= 52 && y <= (52 + sel1H)) {
      if (!_escravo1Ativo) { _escravo1Ativo = true; renderizarTela(); }
      return nullptr;
    }
    if (x >= sel2X && x <= (sel2X + sel2W) && y >= 52 && y <= (52 + sel2H)) {
      if (_escravo1Ativo) { _escravo1Ativo = false; renderizarTela(); }
      return nullptr;
    }
  }

  if (x >= btnProxX && x <= (btnProxX + btnProxW) && y >= btnProxY && y <= (btnProxY + btnProxH)) {
    int maxPaginas = obterTotalPaginasDoModo();
    if (_paginaAtual == (maxPaginas - 1)) {
      _modoOperacao = MODO_INICIAL; _paginaAtual = 0;
      _statusAtual = "Selecione o modo de operacao...";
    } else {
      _paginaAtual++; _statusAtual = "Pagina " + String(_paginaAtual + 1);
    }
    renderizarTela(); return nullptr;
  }

  for (int i = 0; i < TOTAL_BOTOES; i++) {
    bool pertenceAoModo = (_modoOperacao == MODO_LOCAL && i < 12) || (_modoOperacao == MODO_REMOTO && i >= 12);
    if (pertenceAoModo && _botoesAcao[i].pagina == _paginaAtual) {
      if (x >= _botoesAcao[i].x && x <= (_botoesAcao[i].x + _botoesAcao[i].w) && 
          y >= _botoesAcao[i].y && y <= (_botoesAcao[i].y + _botoesAcao[i].h)) {
        return _botoesAcao[i].comando;
      }
    }
  }
  return nullptr;
}

void InterfaceCentral::desligarDisplayFisico() {
  TFT_eSPI& tft = _display.getTftDriver();
  tft.fillScreen(TFT_BLACK); // Apaga os pixels pintando a tela de preto
  digitalWrite(21, LOW);     // Corta fisicamente o Backlight da tela (Pino 21) para economizar bateria
}
