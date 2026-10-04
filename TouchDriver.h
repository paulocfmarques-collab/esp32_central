#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <XPT2046_Touchscreen.h>
#include "LayoutDatabase.h"

class TouchDriver {
public:
    TouchDriver();
    void inicializar();
    bool verificarToqueLegitimo(int& xOut, int& yOut);

private:
    XPT2046_Touchscreen _ts;
    SPIClass _touchSPI;
    
    unsigned long _ultimoTouchProcessado;
    int _ultimoXEstavel;
    int _ultimoYEstavel;
    unsigned long _tempoValidacaoGeometrica;
};

#endif
