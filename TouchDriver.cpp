#include "TouchDriver.h"
#include "Botao.h" // Pega os pinos XPT compartilhados

TouchDriver::TouchDriver() 
  : _touchSPI(HSPI), _ts(XPT_CS), _ultimoTouchProcessado(0), 
    _ultimoXEstavel(-1), _ultimoYEstavel(-1), _tempoValidacaoGeometrica(0) {}

void TouchDriver::inicializar() {
    _touchSPI.begin(XPT_CLK, XPT_MISO, XPT_MOSI, XPT_CS);
    _ts.begin(_touchSPI);
    _ts.setRotation(1);
}

bool TouchDriver::verificarToqueLegitimo(int& xOut, int& yOut) {
    if (!_ts.touched()) {
        _ultimoXEstavel = -1; _ultimoYEstavel = -1;
        return false;
    }

    TS_Point p = _ts.getPoint();
    unsigned long agora = millis();

    // Rejeita assinaturas de pressão falsas/fracas
    if (p.z < 650) return false;

    int xCalc = map(p.x, 300, 3900, 0, SCREEN_W);
    int yCalc = map(p.y, 200, 3700, 0, SCREEN_H);

    // FILTRO ANTI-FANTASMA: Compara desvio da janela geométrica anterior
    if (_ultimoXEstavel == -1 || abs(xCalc - _ultimoXEstavel) > 8 || abs(yCalc - _ultimoYEstavel) > 8) {
        _ultimoXEstavel = xCalc;
        _ultimoYEstavel = yCalc;
        _tempoValidacaoGeometrica = agora;
        return false;
    }

    // Exige persistência de ancoragem por mais de 35ms contínuos
    if (agora - _tempoValidacaoGeometrica < 35) return false;

    // Filtro contra cliques duplicados rápidos
    if (agora - _ultimoTouchProcessado < 450) return false;
    _ultimoTouchProcessado = agora;

    // Trava de bordas estáticas
    if (xCalc <= 2 || xCalc >= (SCREEN_W - 2) || yCalc <= 2 || yCalc >= (SCREEN_H - 2)) return false;

    xOut = xCalc;
    yOut = yCalc;
    return true;
}
