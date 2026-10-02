#include "Display.h"

Display::Display() : _tft(TFT_eSPI()) {}

void Display::inicializar() {
  pinMode(21, OUTPUT); 
  digitalWrite(21, HIGH); // Liga o backlight
  _tft.init(); 
  _tft.setRotation(1); 
  _tft.invertDisplay(true); 
  _tft.fillScreen(TFT_BLACK);
}

TFT_eSPI& Display::getTftDriver() { 
  return _tft; 
}

void Display::mostrarMensagemCentral(const char* msg, uint16_t cor) {
  _tft.fillScreen(TFT_BLACK);
  _tft.setTextColor(cor); 
  _tft.setTextSize(2); 
  _tft.setTextDatum(MC_DATUM);
  _tft.drawString(msg, SCREEN_W / 2, SCREEN_H / 2);
}
