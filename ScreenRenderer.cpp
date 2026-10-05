#include "ScreenRenderer.h"
#include "NTPUtil.h"
#include "UdpComm.h"
#include <WiFi.h>

// A Forward Declaration instável antiga foi completamente removida daqui!

ScreenRenderer::ScreenRenderer(Display& displayHardware) : _display(displayHardware) {}

void ScreenRenderer::limparPainelConteudo() {
    _display.getTftDriver().fillRect(0, 43, SCREEN_W, SCREEN_H - 43, TFT_BLACK);
}

void ScreenRenderer::desenharCabecalhoFixo() {
    TFT_eSPI& tft = _display.getTftDriver();
    tft.fillRect(0, 0, SCREEN_W, 42, 0x2103);
    tft.setTextColor(TFT_GOLD); tft.setTextSize(2); tft.setTextDatum(TL_DATUM);
    tft.drawString(" CENTRAL UDP", 10, 12);
}

void ScreenRenderer::desenharMenuIps(const String& ipEscravo1, const String& ipEscravo2) {
    TFT_eSPI& tft = _display.getTftDriver();
    extern UdpComm udp; 
    
    tft.setTextDatum(MC_DATUM);
    
    // 1. Botão Computador Local Centralizado no Topo (Sempre Azul)
    tft.fillRoundRect(LayoutDatabase::btnLocalX, LayoutDatabase::btnLocalY, LayoutDatabase::btnLocalW, LayoutDatabase::btnLocalH, 6, TFT_BLUE);
    tft.drawRoundRect(LayoutDatabase::btnLocalX, LayoutDatabase::btnLocalY, LayoutDatabase::btnLocalW, LayoutDatabase::btnLocalH, 6, TFT_WHITE);
    tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
    tft.drawString("COMPUTADOR LOCAL", LayoutDatabase::btnLocalX + (LayoutDatabase::btnLocalW / 2), LayoutDatabase::btnLocalY + (LayoutDatabase::btnLocalH / 2));

    tft.setTextSize(1); tft.setTextColor(TFT_YELLOW);
    tft.drawString("--- MONITORAMENTO DE PING ASSINCRONO ---", SCREEN_W / 2, 88);

    // 2. Renderização Dinâmica e Segura do ESP 1
    bool esp1Online = LayoutDatabase::statusEscravos[1];
    uint16_t corEsp1 = esp1Online ? 0x07E0 : 0xF800;
    tft.fillRoundRect(LayoutDatabase::btnIp1X, LayoutDatabase::btnIp1Y, LayoutDatabase::btnIp1W, LayoutDatabase::btnIp1H, 4, corEsp1);
    tft.drawRoundRect(LayoutDatabase::btnIp1X, LayoutDatabase::btnIp1Y, LayoutDatabase::btnIp1W, LayoutDatabase::btnIp1H, 4, TFT_WHITE);
    tft.setTextColor(esp1Online ? TFT_BLACK : TFT_WHITE); 
    tft.drawString("ESP 1: " + udp.obterIpEscravo(1), LayoutDatabase::btnIp1X + (LayoutDatabase::btnIp1W / 2), LayoutDatabase::btnIp1Y + (LayoutDatabase::btnIp1H / 2));

    // 3. Renderização Dinâmica e Segura do ESP 2
    bool esp2Online = LayoutDatabase::statusEscravos[2];
    uint16_t corEsp2 = esp2Online ? 0x07E0 : 0xF800;
    tft.fillRoundRect(LayoutDatabase::btnIp2X, LayoutDatabase::btnIp2Y, LayoutDatabase::btnIp2W, LayoutDatabase::btnIp2H, 4, corEsp2);
    tft.drawRoundRect(LayoutDatabase::btnIp2X, LayoutDatabase::btnIp2Y, LayoutDatabase::btnIp2W, LayoutDatabase::btnIp2H, 4, TFT_WHITE);
    tft.setTextColor(esp2Online ? TFT_BLACK : TFT_WHITE);
    tft.drawString("ESP 2: " + udp.obterIpEscravo(2), LayoutDatabase::btnIp2X + (LayoutDatabase::btnIp2W / 2), LayoutDatabase::btnIp2Y + (LayoutDatabase::btnIp2H / 2));

    // 4. Renderização Dinâmica e Segura do ESP 3
    bool esp3Online = LayoutDatabase::statusEscravos[3];
    uint16_t corEsp3 = esp3Online ? 0x07E0 : 0xF800;
    tft.fillRoundRect(LayoutDatabase::btnIp3X, LayoutDatabase::btnIp3Y, LayoutDatabase::btnIp3W, LayoutDatabase::btnIp3H, 4, corEsp3);
    tft.drawRoundRect(LayoutDatabase::btnIp3X, LayoutDatabase::btnIp3Y, LayoutDatabase::btnIp3W, LayoutDatabase::btnIp3H, 4, TFT_WHITE);
    tft.setTextColor(esp3Online ? TFT_BLACK : TFT_WHITE);
    tft.drawString("ESP 3: " + udp.obterIpEscravo(3), LayoutDatabase::btnIp3X + (LayoutDatabase::btnIp3W / 2), LayoutDatabase::btnIp3Y + (LayoutDatabase::btnIp3H / 2));

    // 5. Renderização Dinâmica e Segura do ESP 4
    bool esp4Online = LayoutDatabase::statusEscravos[4];
    uint16_t corEsp4 = esp4Online ? 0x07E0 : 0xF800;
    tft.fillRoundRect(LayoutDatabase::btnIp4X, LayoutDatabase::btnIp4Y, LayoutDatabase::btnIp4W, LayoutDatabase::btnIp4H, 4, corEsp4);
    tft.drawRoundRect(LayoutDatabase::btnIp4X, LayoutDatabase::btnIp4Y, LayoutDatabase::btnIp4W, LayoutDatabase::btnIp4H, 4, TFT_WHITE);
    tft.setTextColor(esp4Online ? TFT_BLACK : TFT_WHITE);
    tft.drawString("ESP 4: " + udp.obterIpEscravo(4), LayoutDatabase::btnIp4X + (LayoutDatabase::btnIp4W / 2), LayoutDatabase::btnIp4Y + (LayoutDatabase::btnIp4H / 2));
}

void ScreenRenderer::desenharPainelPaginas(int modo, int escravoAlvo, int paginaAtual, const String& ip1, const String& ip2, const String& status) {
    TFT_eSPI& tft = _display.getTftDriver();
    extern UdpComm udp;
    
    if (modo == 2) { // MODO_REMOTO
        tft.fillRect(15, 52, 290, 35, 0x3186);
        tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.setTextDatum(MC_DATUM);
        String targetStr = "ESP " + String(escravoAlvo) + " (" + udp.obterIpEscravo(escravoAlvo) + ")";
        tft.drawString("CONTROLE REMOTO UDP -> " + targetStr, 160, 69);
    } else {
        tft.fillRect(15, 52, 290, 35, 0x10A2);
        tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.setTextDatum(MC_DATUM);
        tft.drawString("PAINEL DE COMANDOS INTERNOS DO MESTRE", 160, 69);
    }

    tft.setTextSize(1); tft.setTextDatum(MC_DATUM);
    for (int i = 0; i < TOTAL_BOTOES; i++) {
        bool pertenceAoModo = (modo == 1 && i < 30) || (modo == 2 && i >= 30);
        if (pertenceAoModo && LayoutDatabase::botoesAcao[i].pagina == paginaAtual) {
            tft.fillRoundRect(LayoutDatabase::botoesAcao[i].x, LayoutDatabase::botoesAcao[i].y, LayoutDatabase::botoesAcao[i].w, LayoutDatabase::botoesAcao[i].h, 3, LayoutDatabase::botoesAcao[i].cor);
            tft.drawRoundRect(LayoutDatabase::botoesAcao[i].x, LayoutDatabase::botoesAcao[i].y, LayoutDatabase::botoesAcao[i].w, LayoutDatabase::botoesAcao[i].h, 3, TFT_WHITE);
            
            tft.setTextColor((LayoutDatabase::botoesAcao[i].cor == 0x07E0 || LayoutDatabase::botoesAcao[i].cor == 0xFD20) ? TFT_BLACK : TFT_WHITE);
            tft.drawString(LayoutDatabase::botoesAcao[i].label, LayoutDatabase::botoesAcao[i].x + (LayoutDatabase::botoesAcao[i].w / 2), LayoutDatabase::botoesAcao[i].y + (LayoutDatabase::botoesAcao[i].h / 2));
        }
    }

    tft.fillRoundRect(LayoutDatabase::btnProxX, LayoutDatabase::btnProxY, LayoutDatabase::btnProxW, LayoutDatabase::btnProxH, 4, 0x3186);
    tft.drawRoundRect(LayoutDatabase::btnProxX, LayoutDatabase::btnProxY, LayoutDatabase::btnProxW, LayoutDatabase::btnProxH, 4, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    
    if (paginaAtual == 4) {
        tft.fillRoundRect(LayoutDatabase::btnProxX, LayoutDatabase::btnProxY, LayoutDatabase::btnProxW, LayoutDatabase::btnProxH, 4, TFT_RED);
        tft.drawRoundRect(LayoutDatabase::btnProxX, LayoutDatabase::btnProxY, LayoutDatabase::btnProxW, LayoutDatabase::btnProxH, 4, TFT_WHITE);
        tft.drawString("VOLTAR AO MENU PRINCIPAL DE IPS", LayoutDatabase::btnProxX + (LayoutDatabase::btnProxW / 2), LayoutDatabase::btnProxY + (LayoutDatabase::btnProxH / 2));
    } else {
        String nomeModo = (modo == 1) ? "LOCAIS" : "REMOTOS";
        tft.drawString("PROXIMOS COMANDOS " + nomeModo + " (PAG " + String(paginaAtual + 1) + "/5)", LayoutDatabase::btnProxX + (LayoutDatabase::btnProxW / 2), LayoutDatabase::btnProxY + (LayoutDatabase::btnProxH / 2));
    }
    desenharRodape(status, TFT_LIGHTGREY);
}

void ScreenRenderer::desenharTelaResposta(const String& msg, int& yTerminalOut) {
    TFT_eSPI& tft = _display.getTftDriver();
    tft.fillRect(0, 43, SCREEN_W, SCREEN_H - 43, TFT_BLACK);
    
    int tamanhoFonte = 1; 
    int espacamento = 14;
    yTerminalOut = 52; 

    tft.setTextColor(TFT_GREEN); 
    tft.setTextSize(tamanhoFonte); 
    tft.setTextDatum(TL_DATUM);
    
    int indice = 0;
    while (indice < msg.length()) {
        int quebra = msg.indexOf('\n', indice);
        String token = (quebra == -1) ? msg.substring(indice) : msg.substring(indice, quebra);
        indice = (quebra == -1) ? msg.length() : quebra + 1;
        token.trim();
        
        if (token.length() > 0) { 
            // 🔥 Margem horizontal reduzida para 36 caracteres para não bater nos botões de Scroll à direita
            if (token.length() > 36) {
                token = token.substring(0, 33) + "...";
            }
            tft.drawString(token, 15, yTerminalOut); 
            yTerminalOut += espacamento; 
        }
    }
    
    tft.setTextColor(TFT_DARKGREY); tft.setTextSize(1); tft.setTextDatum(BC_DATUM);
    tft.drawString("[ Toque no texto ou aguarde 30s para voltar ]", 140, SCREEN_H - 15);
    
    // Desenha as setas gráficas na lateral direita
    desenharBotoesScroll();
}

void ScreenRenderer::desenharBotoesScroll() {
    TFT_eSPI& tft = _display.getTftDriver();
    
    // Botão Seta Para Cima (▲) - Topo Direito
    tft.fillRoundRect(275, 50, 35, 45, 4, 0x2103);
    tft.drawRoundRect(275, 50, 35, 45, 4, TFT_WHITE);
    tft.setTextColor(TFT_GOLD); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
    tft.drawString("^", 292, 72);
    
    // Botão Seta Para Baixo (▼) - Centro/Inferior Direito
    tft.fillRoundRect(275, 110, 35, 45, 4, 0x2103);
    tft.drawRoundRect(275, 110, 35, 45, 4, TFT_WHITE);
    tft.drawString("v", 292, 130);
}

void ScreenRenderer::renderizarNovaLinhaResposta(const String& msg, int& yTerminalInOut) {
    TFT_eSPI& tft = _display.getTftDriver();
    
    int quebra = msg.indexOf('\n');
    String token = (quebra == -1) ? msg : msg.substring(0, quebra);
    
    if (token.length() > 0) {
        int tamanhoFonte = 1;
        int incrementoY = 14;
        
        tft.setTextColor(TFT_GREEN); 
        tft.setTextSize(tamanhoFonte); 
        tft.setTextDatum(TL_DATUM);
        
        // Proteção contra estouro vertical da tela (Scroll Virtual do buffer)
        if (yTerminalInOut > 205) { 
            tft.fillRect(0, 43, SCREEN_W, SCREEN_H - 43 - 18, TFT_BLACK); 
            yTerminalInOut = 60; 
        }
        
        // Corta horizontalmente se a mensagem remota for absurdamente longa
        if (token.length() > 42) {
            token = token.substring(0, 39) + "...";
        }
        
        tft.drawString(token, 15, yTerminalInOut); 
        yTerminalInOut += incrementoY;
    }
}

void ScreenRenderer::atualizarRelogio(unsigned long& ultimoTimestamp) {
    unsigned long agora = millis();
    if (agora - ultimoTimestamp >= 1000) {
        ultimoTimestamp = agora; TFT_eSPI& tft = _display.getTftDriver();
        tft.fillRect(230, 24, 85, 14, 0x2103); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.setTextDatum(TR_DATUM);
        tft.drawString(ntp.getSomenteHora(), 310, 24);
    }
}

void ScreenRenderer::atualizarSinalWifi(unsigned long& ultimoTimestamp) {
    unsigned long agora = millis();
    if (agora - ultimoTimestamp >= 5000 || ultimoTimestamp == 0) {
        ultimoTimestamp = agora; TFT_eSPI& tft = _display.getTftDriver();
        long rssi = WiFi.RSSI(); uint16_t cor = TFT_GREEN; int barras = 4;
        if (rssi < -85) { cor = TFT_RED; barras = 1; }
        else if (rssi < -75) { cor = TFT_YELLOW; barras = 2; }
        else if (rssi < -65) { cor = TFT_GREEN; barras = 3; }
        tft.fillRect(200, 6, 115, 16, 0x2103); tft.setTextSize(1);
        String wifiStr = String(rssi) + "dBm"; int larg = tft.textWidth(wifiStr);
        tft.setTextColor(cor); tft.setTextDatum(TR_DATUM); tft.drawString(wifiStr, 310, 8);
        int bx = 310 - larg - 22;
        for (int b = 1; b <= 4; b++) {
            tft.fillRect(bx + (b * 4), 18 - (b * 3), 2, b * 3, (b <= barras) ? cor : TFT_DARKGREY);
        }
    }
}

void ScreenRenderer::desenharRodape(const String& msg, uint16_t cor) {
    String limpa = (msg.length() > 45) ? msg.substring(0, 42) + "..." : msg;
    TFT_eSPI& tft = _display.getTftDriver(); tft.fillRect(0, 222, SCREEN_W, 18, 0x1082);
    tft.setTextColor(cor); tft.setTextSize(1); tft.setTextDatum(MC_DATUM);
    tft.drawString(limpa, SCREEN_W / 2, 231);
}
