#ifndef SCREEN_RENDERER_H
#define SCREEN_RENDERER_H

#include "Display.h"
#include "LayoutDatabase.h"

class ScreenRenderer
{
  public:
    ScreenRenderer(Display& displayHardware);

    TFT_eSPI& getDriver()
    {
        return _display.getTftDriver();
    }

    void limparPainelConteudo();
    void desenharCabecalhoFixo();
    void desenharMenuIps(const String& ipEscravo1, const String& ipEscravo2);
    void desenharPainelPaginas(int modo, int escravoAlvo, int paginaAtual, const String& ip1,
                               const String& ip2, const String& status);
    void desenharTelaResposta(const String& msg, int& yTerminalOut);
    void desenharBotoesScroll();
    void renderizarNovaLinhaResposta(const String& msg, int& yTerminalInOut);
    void atualizarRelogio(unsigned long& ultimoTimestamp);
    void atualizarSinalWifi(unsigned long& ultimoTimestamp);
    void desenharRodape(const String& msg, uint16_t cor);
    void desenharDiagnosticos(int pagina, bool atualizar = false);
    void coletarSinalWifi();
    void iniciarMatrix();
    void atualizarMatrix();

  private:
    void desenharNavegacao();
    struct ColunaMatrix
    {
        int cabeca;
        uint8_t comprimento;
        uint8_t intervalo;
        uint8_t passo;
    };

    ColunaMatrix _matrix[40] = {};
    unsigned long _ultimoMatrixTS = 0;
    int _historicoRSSI[60] = {};
    int _proximaAmostra = 0;
    int _totalAmostras = 0;
    unsigned long _ultimaAmostraTS = 0;
    Display& _display;
    TFT_eSprite _painelDiagnostico;
};

#endif
