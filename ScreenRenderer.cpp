#include "ScreenRenderer.h"
#include "NTPUtil.h"
#include "UdpComm.h"
#include <WiFi.h>
#include "ClimaManager.h"
#include "SdManager.h"

// A Forward Declaration instável antiga foi completamente removida daqui!

ScreenRenderer::ScreenRenderer(Display& displayHardware)
    : _display(displayHardware), _painelDiagnostico(&displayHardware.getTftDriver())
{
}

void ScreenRenderer::iniciarMatrix()
{
    _painelDiagnostico.deleteSprite();
    auto& tft = getDriver();
    tft.fillScreen(TFT_BLACK);
    for (auto& coluna : _matrix)
    {
        coluna.cabeca = -random(1, 25);
        coluna.comprimento = random(6, 16);
        coluna.intervalo = random(1, 4);
        coluna.passo = 0;
    }
    _ultimoMatrixTS = millis();
}

void ScreenRenderer::atualizarMatrix()
{
    if (millis() - _ultimoMatrixTS < 70)
        return;
    _ultimoMatrixTS = millis();
    auto& tft = getDriver();
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    const char caracteres[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ@#$%&*";
    const int linhas = SCREEN_H / 10;
    tft.startWrite();
    for (int i = 0; i < SCREEN_W / 8 && i < 40; ++i)
    {
        auto& coluna = _matrix[i];
        if (++coluna.passo < coluna.intervalo)
            continue;
        coluna.passo = 0;
        ++coluna.cabeca;
        // Apaga somente a cauda: nenhuma limpeza completa durante a animacao.
        int cauda = coluna.cabeca - coluna.comprimento;
        if (cauda >= 0 && cauda < linhas)
            tft.fillRect(i * 8, cauda * 10, 8, 10, TFT_BLACK);
        for (int distancia = 0; distancia < coluna.comprimento; ++distancia)
        {
            int linha = coluna.cabeca - distancia;
            if (linha < 0 || linha >= linhas)
                continue;
            uint16_t cor = distancia == 0 ? 0xCFF9 : tft.color565(0, 220 - distancia * 12, 20);
            tft.setTextColor(cor, TFT_BLACK);
            char simbolo[2] = {caracteres[random(sizeof(caracteres) - 1)], 0};
            tft.drawString(simbolo, i * 8, linha * 10);
        }
        if (cauda >= linhas)
            coluna.cabeca = -random(1, 15);
    }
    tft.endWrite();
}

void ScreenRenderer::limparPainelConteudo()
{
    _painelDiagnostico.deleteSprite();
    _display.getTftDriver().fillRect(0, 43, SCREEN_W, SCREEN_H - 43, TFT_BLACK);
}

void ScreenRenderer::desenharCabecalhoFixo()
{
    TFT_eSPI& tft = _display.getTftDriver();
    tft.fillRect(0, 0, SCREEN_W, 42, 0x2103);
    tft.setTextColor(TFT_GOLD);
    tft.setTextSize(2);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(" CENTRAL UDP", 10, 12);
}

void ScreenRenderer::desenharMenuIps(const String& ipEscravo1, const String& ipEscravo2)
{
    TFT_eSPI& tft = _display.getTftDriver();
    extern UdpComm udp;

    tft.setTextDatum(MC_DATUM);

    // 1. Botão Computador Local Centralizado no Topo (Sempre Azul)
    tft.fillRoundRect(LayoutDatabase::btnLocalX, LayoutDatabase::btnLocalY,
                      LayoutDatabase::btnLocalW, LayoutDatabase::btnLocalH, 6, TFT_BLUE);
    tft.drawRoundRect(LayoutDatabase::btnLocalX, LayoutDatabase::btnLocalY,
                      LayoutDatabase::btnLocalW, LayoutDatabase::btnLocalH, 6, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.drawString("COMPUTADOR LOCAL", LayoutDatabase::btnLocalX + (LayoutDatabase::btnLocalW / 2),
                   LayoutDatabase::btnLocalY + (LayoutDatabase::btnLocalH / 2));

    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("--- MONITORAMENTO DE PING ASSINCRONO ---", SCREEN_W / 2, 88);

    // Tela inicial: cores e dimensoes originais, conforme o estado do ESP.
    auto desenharEsp = [&](int id, int x, int y, int w, int h) {
        bool online = LayoutDatabase::statusEscravos[id];
        tft.fillRoundRect(x, y, w, h, 4, online ? TFT_GREEN : TFT_RED);
        tft.drawRoundRect(x, y, w, h, 4, TFT_WHITE);
        tft.setTextColor(online ? TFT_BLACK : TFT_WHITE);
        tft.setTextSize(1);
        tft.drawString("ESP " + String(id) + ": " + udp.obterIpEscravo(id), x + w / 2, y + h / 2);
    };
    desenharEsp(1, LayoutDatabase::btnIp1X, LayoutDatabase::btnIp1Y,
                LayoutDatabase::btnIp1W, LayoutDatabase::btnIp1H);
    desenharEsp(2, LayoutDatabase::btnIp2X, LayoutDatabase::btnIp2Y,
                LayoutDatabase::btnIp2W, LayoutDatabase::btnIp2H);
    desenharEsp(3, LayoutDatabase::btnIp3X, LayoutDatabase::btnIp3Y,
                LayoutDatabase::btnIp3W, LayoutDatabase::btnIp3H);
    desenharEsp(4, LayoutDatabase::btnIp4X, LayoutDatabase::btnIp4Y,
                LayoutDatabase::btnIp4W, LayoutDatabase::btnIp4H);
    tft.fillRoundRect(LayoutDatabase::btnDiagX, LayoutDatabase::btnDiagY, LayoutDatabase::btnDiagW,
                      LayoutDatabase::btnDiagH, 6, TFT_DARKCYAN);
    tft.drawRoundRect(LayoutDatabase::btnDiagX, LayoutDatabase::btnDiagY, LayoutDatabase::btnDiagW,
                      LayoutDatabase::btnDiagH, 6, TFT_CYAN);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.drawString("Diagnosticos", SCREEN_W / 2,
                   LayoutDatabase::btnDiagY + LayoutDatabase::btnDiagH / 2);
}

void ScreenRenderer::coletarSinalWifi()
{
    unsigned long agora = millis();
    if (_totalAmostras && agora - _ultimaAmostraTS < 1000)
        return;
    _ultimaAmostraTS = agora;
    _historicoRSSI[_proximaAmostra] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -127;
    _proximaAmostra = (_proximaAmostra + 1) % 60;
    if (_totalAmostras < 60)
        ++_totalAmostras;
}

void ScreenRenderer::desenharDiagnosticos(int pagina, bool atualizar)
{
    // Monta o quadro em memoria: o display recebe os pixels prontos,
    // sem exibir a limpeza e cada etapa do desenho.
    if (!_painelDiagnostico.created())
    {
        // O quadro de 8 bits ocupa 46 KB. Nao consumir a reserva da rede,
        // e da consulta de clima so porque a alocacao cabe.
        uint32_t livre = ESP.getFreeHeap();
        _painelDiagnostico.setColorDepth(livre >= 100000 ? 8 : livre >= 70000 ? 4 : 1);
        if (!_painelDiagnostico.createSprite(SCREEN_W, 145))
        {
            // Alternativa de baixo consumo de RAM para continuar navegando.
            _painelDiagnostico.setColorDepth(1);
            if (!_painelDiagnostico.createSprite(SCREEN_W, 145))
                return;
        }
        _painelDiagnostico.setOrigin(0, -43);
    }
    TFT_eSprite& tft = _painelDiagnostico;
    const char* titulos[] = {"DISPOSITIVO",   "TEMPO / RELOGIO",      "REDE WI-FI",
                             "SINAL DA REDE", "CLIMA / PORTO ALEGRE", "CARTAO SD"};
    tft.fillRect(0, 43, SCREEN_W, 145, TFT_BLACK);
    tft.fillRoundRect(10, 48, 300, 25, 4, 0x10A2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(String(titulos[pagina]) + "  " + String(pagina + 1) + "/6", 160, 60);
    auto linha = [&](int y, String texto, uint16_t cor = TFT_WHITE)
    {
        if (texto.length() > 48)
            texto = texto.substring(0, 45) + "...";
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(cor);
        tft.drawString(texto, 16, y);
    };
    bool conectado = WiFi.status() == WL_CONNECTED;
    if (pagina == 0)
    {
        linha(82,
              String("Modelo: ") + ESP.getChipModel() + " / rev " + String(ESP.getChipRevision()));
        linha(99, "CPU: " + String(ESP.getCpuFreqMHz()) + " MHz / " + String(ESP.getChipCores()) +
                      " nucleos");
        linha(116, "RAM livre: " + String(ESP.getFreeHeap() / 1024) + " KB");
        linha(133, "RAM minima livre: " + String(ESP.getMinFreeHeap() / 1024) + " KB");
        linha(150, "Flash: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB / firmware " +
                       String(ESP.getSketchSize() / 1024) + " KB");
        linha(167, String("Build: ") + __DATE__ + " " + __TIME__);
    }
    else if (pagina == 1)
    {
        struct tm hora;
        bool sincronizado = getLocalTime(&hora, 0);
        char data[24] = "--/--/---- --:--:--";
        if (sincronizado)
            strftime(data, sizeof(data), "%d/%m/%Y %H:%M:%S", &hora);
        linha(82, sincronizado ? "NTP: SINCRONIZADO" : "NTP: AGUARDANDO",
              sincronizado ? TFT_GREEN : TFT_YELLOW);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE);
        tft.drawString(data, 160, 115);
        unsigned long segundos = millis() / 1000;
        char uptime[48];
        snprintf(uptime, sizeof(uptime), "Ligado: %lud %02lu:%02lu:%02lu", segundos / 86400,
                 segundos / 3600 % 24, segundos / 60 % 60, segundos % 60);
        linha(146, uptime);
        linha(166, "Fuso: " + String(getenv("TZ") ? getenv("TZ") : "--"));
    }
    else if (pagina == 2)
    {
        linha(80, conectado ? "Wi-Fi: CONECTADO" : "Wi-Fi: DESCONECTADO",
              conectado ? TFT_GREEN : TFT_RED);
        linha(95, "SSID: " + WiFi.SSID());
        linha(110, "IP: " + WiFi.localIP().toString() + "  Canal: " + String(WiFi.channel()));
        linha(125, "Gateway: " + WiFi.gatewayIP().toString());
        linha(140, "Mascara: " + WiFi.subnetMask().toString());
        linha(155, "DNS: " + WiFi.dnsIP().toString());
        linha(170, "MAC: " + WiFi.macAddress());
    }
    else if (pagina == 3)
    {
        int rssi = conectado ? WiFi.RSSI() : -127;
        int qualidade = conectado ? constrain(2 * (rssi + 100), 0, 100) : 0;
        uint16_t cor = rssi > -70 ? TFT_GREEN : rssi > -80 ? TFT_YELLOW : TFT_RED;
        linha(79,
              conectado ? "RSSI: " + String(rssi) + " dBm / " + String(qualidade) + "% estimado"
                        : "Sem conexao Wi-Fi",
              cor);
        tft.drawRect(43, 98, 260, 67, TFT_DARKGREY);
        linha(98, "-40", TFT_LIGHTGREY);
        linha(153, "-100", TFT_LIGHTGREY);
        tft.drawFastHLine(44, 131, 258, 0x2103);
        int primeiro = (_proximaAmostra + 60 - _totalAmostras) % 60;
        for (int i = 1; i < _totalAmostras; ++i)
        {
            int a = _historicoRSSI[(primeiro + i - 1) % 60];
            int b = _historicoRSSI[(primeiro + i) % 60];
            if (a == -127 || b == -127)
                continue;
            tft.drawLine(44 + (i - 1) * 258 / 59, map(constrain(a, -100, -40), -100, -40, 163, 100),
                         44 + i * 258 / 59, map(constrain(b, -100, -40), -100, -40, 163, 100), cor);
        }
        linha(174, "Historico: 60 amostras / 1 segundo", TFT_LIGHTGREY);
    }
    else if (pagina == 4)
    {
        auto clima = ClimaManager::snapshot();
        if (!clima.sincronizado)
        {
            linha(90,
                  clima.ultimaFalhou ? "Falha na consulta. Tentando novamente..."
                                     : "Consultando clima de Porto Alegre...",
                  TFT_YELLOW);
            linha(115, conectado ? "Aguarde a primeira atualizacao." : "Aguardando conexao Wi-Fi.");
        }
        else
        {
            linha(80, ClimaManager::obterTextoCondicao(), TFT_CYAN);
            linha(97, "Temperatura: " + String(clima.temperatura, 1) +
                          " C / Umidade: " + String(clima.umidade) + "%");
            if (clima.previsaoValida)
            {
                linha(114, "Min: " + String(clima.minima, 1) +
                               " C / Max: " + String(clima.maxima, 1) + " C");
                linha(131, "Chance de chuva: " + String(clima.chuva) + "% / " + String(clima.data));
            }
            else
                linha(114, "Previsao diaria indisponivel", TFT_YELLOW);
            linha(148,
                  "Atualizado ha " + String((millis() - clima.ultimaAtualizacao) / 60000) + " min",
                  clima.ultimaFalhou ? TFT_YELLOW : TFT_GREEN);
            linha(165,
                  clima.ultimaFalhou ? "Falha ao atualizar: exibindo ultima leitura"
                                     : "Fonte: Open-Meteo / atualiza a cada 15 min",
                  TFT_LIGHTGREY);
        }
    }
    if (pagina == 5)
    {
        auto sd = SdManager::snapshot();
        linha(80, sd.pronto ? "SD: PRONTO / " + sd.tipo : "SD: INDISPONIVEL",
              sd.pronto ? TFT_GREEN : TFT_RED);
        linha(95, "Capacidade: " + String(double(sd.capacidade) / (1024 * 1024 * 1024), 2) + " GB");
        linha(
            110,
            "Usado: " + String(double(sd.usado) / (1024 * 1024), 1) + " MB / livre: " +
                String(double(sd.total >= sd.usado ? sd.total - sd.usado : 0) / (1024 * 1024), 1) +
                " MB");
        linha(125, "SPI: " + String(sd.frequencia / 1000) + " kHz / CS5 CLK18 MISO19 MOSI23");
        linha(140, "/log.txt: " + String(sd.tamanhoLog / 1024.0, 1) +
                       " KB / registros: " + String(sd.registros));
        linha(155, "Falhas de log: " + String(sd.falhasLog), sd.falhasLog ? TFT_YELLOW : TFT_WHITE);
        linha(170, sd.erro.length() ? sd.erro : "Log ativo / informacoes a cada 30s",
              sd.erro.length() ? TFT_YELLOW : TFT_LIGHTGREY);
    }
    _painelDiagnostico.pushSprite(0, 43);
    // Botoes e rodape sao fixos; so redesenha ao abrir ou trocar de pagina.
    if (atualizar)
        return;
    desenharNavegacao();
    desenharRodape("Sem toque por 30s: volta ao menu", TFT_LIGHTGREY);
}

void ScreenRenderer::desenharNavegacao()
{
    TFT_eSPI& display = _display.getTftDriver();
    const int xs[] = {10, 110, 220};
    const int ws[] = {90, 100, 90};
    const char* labels[] = {"< ANTERIOR", "MENU", "PROXIMA >"};
    for (int i = 0; i < 3; ++i)
    {
        display.fillRoundRect(xs[i], 190, ws[i], 26, 4, i == 1 ? TFT_DARKCYAN : 0x3186);
        display.drawRoundRect(xs[i], 190, ws[i], 26, 4, TFT_WHITE);
        display.setTextDatum(MC_DATUM);
        display.setTextSize(1);
        display.setTextColor(TFT_WHITE);
        display.drawString(labels[i], xs[i] + ws[i] / 2, 203);
    }
}

void ScreenRenderer::desenharPainelPaginas(int modo, int escravoAlvo, int paginaAtual,
                                           const String& ip1, const String& ip2,
                                           const String& status)
{
    TFT_eSPI& tft = _display.getTftDriver();
    extern UdpComm udp;

    if (modo == 2)
    { // MODO_REMOTO
        tft.fillRect(15, 52, 290, 35, 0x3186);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(1);
        tft.setTextDatum(MC_DATUM);
        String targetStr =
            "ESP " + String(escravoAlvo) + " (" + udp.obterIpEscravo(escravoAlvo) + ")";
        tft.drawString("CONTROLE REMOTO UDP -> " + targetStr, 160, 69);
    }
    else
    {
        tft.fillRect(15, 52, 290, 35, 0x10A2);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(1);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("PAINEL DE COMANDOS INTERNOS DO MESTRE", 160, 69);
    }

    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    for (int i = 0; i < TOTAL_BOTOES; i++)
    {
        bool pertenceAoModo = (modo == 1 && i < 30) || (modo == 2 && i >= 30);
        if (pertenceAoModo && LayoutDatabase::botoesAcao[i].pagina == paginaAtual)
        {
            tft.fillRoundRect(LayoutDatabase::botoesAcao[i].x, LayoutDatabase::botoesAcao[i].y,
                              LayoutDatabase::botoesAcao[i].w, LayoutDatabase::botoesAcao[i].h, 3,
                              LayoutDatabase::botoesAcao[i].cor);
            tft.drawRoundRect(LayoutDatabase::botoesAcao[i].x, LayoutDatabase::botoesAcao[i].y,
                              LayoutDatabase::botoesAcao[i].w, LayoutDatabase::botoesAcao[i].h, 3,
                              TFT_WHITE);

            tft.setTextColor((LayoutDatabase::botoesAcao[i].cor == 0x07E0 ||
                              LayoutDatabase::botoesAcao[i].cor == 0xFD20)
                                 ? TFT_BLACK
                                 : TFT_WHITE);
            tft.drawString(LayoutDatabase::botoesAcao[i].label,
                           LayoutDatabase::botoesAcao[i].x + (LayoutDatabase::botoesAcao[i].w / 2),
                           LayoutDatabase::botoesAcao[i].y + (LayoutDatabase::botoesAcao[i].h / 2));
        }
    }

    desenharNavegacao();
    desenharRodape(status, TFT_LIGHTGREY);
}

void ScreenRenderer::desenharTelaResposta(const String& msg, int& yTerminalOut)
{
    TFT_eSPI& tft = _display.getTftDriver();
    tft.fillRect(0, 43, SCREEN_W, SCREEN_H - 43, TFT_BLACK);

    int tamanhoFonte = 1;
    int espacamento = 14;
    yTerminalOut = 52;

    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(tamanhoFonte);
    tft.setTextDatum(TL_DATUM);

    int indice = 0;
    while (indice < msg.length())
    {
        int quebra = msg.indexOf('\n', indice);
        String token = (quebra == -1) ? msg.substring(indice) : msg.substring(indice, quebra);
        indice = (quebra == -1) ? msg.length() : quebra + 1;
        token.trim();

        if (token.length() > 0)
        {
            // 🔥 Margem horizontal reduzida para 36 caracteres para não bater nos botões de Scroll
            // à direita
            if (token.length() > 36)
            {
                token = token.substring(0, 33) + "...";
            }
            tft.drawString(token, 15, yTerminalOut);
            yTerminalOut += espacamento;
        }
    }

    tft.setTextColor(TFT_DARKGREY);
    tft.setTextSize(1);
    tft.setTextDatum(BC_DATUM);
    tft.drawString("[ Toque no texto ou aguarde 30s para voltar ]", 140, SCREEN_H - 15);

    // Desenha as setas gráficas na lateral direita
    desenharBotoesScroll();
}

void ScreenRenderer::desenharBotoesScroll()
{
    TFT_eSPI& tft = _display.getTftDriver();

    // Botão Seta Para Cima (▲) - Topo Direito
    tft.fillRoundRect(275, 50, 35, 45, 4, 0x2103);
    tft.drawRoundRect(275, 50, 35, 45, 4, TFT_WHITE);
    tft.setTextColor(TFT_GOLD);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("^", 292, 72);

    // Botão Seta Para Baixo (▼) - Centro/Inferior Direito
    tft.fillRoundRect(275, 110, 35, 45, 4, 0x2103);
    tft.drawRoundRect(275, 110, 35, 45, 4, TFT_WHITE);
    tft.drawString("v", 292, 130);
}

void ScreenRenderer::renderizarNovaLinhaResposta(const String& msg, int& yTerminalInOut)
{
    TFT_eSPI& tft = _display.getTftDriver();

    int quebra = msg.indexOf('\n');
    String token = (quebra == -1) ? msg : msg.substring(0, quebra);

    if (token.length() > 0)
    {
        int tamanhoFonte = 1;
        int incrementoY = 14;

        tft.setTextColor(TFT_GREEN);
        tft.setTextSize(tamanhoFonte);
        tft.setTextDatum(TL_DATUM);

        // Proteção contra estouro vertical da tela (Scroll Virtual do buffer)
        if (yTerminalInOut > 205)
        {
            tft.fillRect(0, 43, SCREEN_W, SCREEN_H - 43 - 18, TFT_BLACK);
            yTerminalInOut = 60;
        }

        // Corta horizontalmente se a mensagem remota for absurdamente longa
        if (token.length() > 42)
        {
            token = token.substring(0, 39) + "...";
        }

        tft.drawString(token, 15, yTerminalInOut);
        yTerminalInOut += incrementoY;
    }
}

void ScreenRenderer::atualizarRelogio(unsigned long& ultimoTimestamp)
{
    unsigned long agora = millis();
    if (agora - ultimoTimestamp >= 1000)
    {
        ultimoTimestamp = agora;
        TFT_eSPI& tft = _display.getTftDriver();
        tft.fillRect(230, 24, 85, 14, 0x2103);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(1);
        tft.setTextDatum(TR_DATUM);
        tft.drawString(ntp.getSomenteHora(), 310, 24);
    }
}

void ScreenRenderer::atualizarSinalWifi(unsigned long& ultimoTimestamp)
{
    unsigned long agora = millis();
    if (agora - ultimoTimestamp >= 5000 || ultimoTimestamp == 0)
    {
        ultimoTimestamp = agora;
        TFT_eSPI& tft = _display.getTftDriver();
        long rssi = WiFi.RSSI();
        uint16_t cor = TFT_GREEN;
        int barras = 4;
        if (rssi < -85)
        {
            cor = TFT_RED;
            barras = 1;
        }
        else if (rssi < -75)
        {
            cor = TFT_YELLOW;
            barras = 2;
        }
        else if (rssi < -65)
        {
            cor = TFT_GREEN;
            barras = 3;
        }
        tft.fillRect(200, 6, 115, 16, 0x2103);
        tft.setTextSize(1);
        String wifiStr = String(rssi) + "dBm";
        int larg = tft.textWidth(wifiStr);
        tft.setTextColor(cor);
        tft.setTextDatum(TR_DATUM);
        tft.drawString(wifiStr, 310, 8);
        int bx = 310 - larg - 22;
        for (int b = 1; b <= 4; b++)
        {
            tft.fillRect(bx + (b * 4), 18 - (b * 3), 2, b * 3, (b <= barras) ? cor : TFT_DARKGREY);
        }
    }
}

void ScreenRenderer::desenharRodape(const String& msg, uint16_t cor)
{
    String limpa = (msg.length() > 45) ? msg.substring(0, 42) + "..." : msg;
    TFT_eSPI& tft = _display.getTftDriver();
    tft.fillRect(0, 222, SCREEN_W, 18, 0x1082);
    tft.setTextColor(cor);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(limpa, SCREEN_W / 2, 231);
}
