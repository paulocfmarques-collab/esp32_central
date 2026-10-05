#ifndef LAYOUT_DATABASE_H
#define LAYOUT_DATABASE_H

#include <Arduino.h>

#define TOTAL_BOTOES 60
#define SCREEN_W  320
#define SCREEN_H  240

struct Botao {
    int x, y, w, h;
    const char* label;
    const char* comando;
    uint16_t cor;
    int pagina;
};

class LayoutDatabase {
public:
    static Botao botoesAcao[TOTAL_BOTOES];
    
    static bool statusEscravos[5]; 

    // Geometria da nova Tela Inicial em Grade de 2 colunas
    static constexpr int btnLocalX = 40,  btnLocalY = 50,  btnLocalW = 240, btnLocalH = 30;
    static constexpr int btnIp1X   = 15,  btnIp1Y   = 100, btnIp1W = 140, btnIp1H = 28; 
    static constexpr int btnIp2X   = 165, btnIp2Y   = 100, btnIp2W = 140, btnIp2H = 28; 
    static constexpr int btnIp3X   = 15,  btnIp3Y   = 135, btnIp3W = 140, btnIp3H = 28; 
    static constexpr int btnIp4X   = 165, btnIp4Y   = 135, btnIp4W = 140, btnIp4H = 28; 
    static constexpr int btnProxX  = 15,  btnProxY  = 190, btnProxW  = 290, btnProxH  = 26;
};

#endif
