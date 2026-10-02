#ifndef BOTAO_H
#define BOTAO_H

#include <Arduino.h>

#define SCREEN_W  320
#define SCREEN_H  240

#define XPT_CLK   25
#define XPT_MISO  39
#define XPT_MOSI  32
#define XPT_CS    33

struct Botao {
  int x, y, w, h;
  const char* label;
  const char* comando;
  uint16_t cor;
  int pagina; // <-- Identifica a página do botão (0 para Pág 1, 1 para Pág 2, etc.)
};

#endif
