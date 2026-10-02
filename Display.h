#ifndef DISPLAY_H
#define DISPLAY_H

#include <TFT_eSPI.h>
#include "Botao.h"

class Display {
private:
  TFT_eSPI _tft;

public:
  Display();
  void inicializar();
  TFT_eSPI& getTftDriver();
  void mostrarMensagemCentral(const char* msg, uint16_t cor = TFT_WHITE);
};

#endif
