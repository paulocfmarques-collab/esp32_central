#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "Display.h"

class WifiConfig {
private:
  WebServer _server;
  Preferences _prefs;
  Display& _display;
  
  static WifiConfig* _instance;

  // Variáveis para salvamento assíncrono seguro
  volatile bool _dadosProntosParaSalvar;
  String _tempSSID;
  String _tempSenha;
  String _tempIp1;
  String _tempIp2;
  String _tempIp3;
  String _tempIp4;

  // Callbacks estáticos para compatibilidade com o servidor Web do ESP32
  static void handleRootCallback();
  static void handleSaveCallback();
  static void handleInfoApiCallback(); 

  // Métodos internos de processamento de rotas HTTP
  void handleRoot();
  void handleSave();
  void handleInfoApi(); 

public:
  WifiConfig(Display& displayRef);
  bool conectar();
  void iniciarPortal();
  void processarPortal();
};

#endif
