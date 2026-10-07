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
  static const int MAX_REDES = 5;
  String _tempSSIDs[MAX_REDES];
  String _tempSenhas[MAX_REDES];
  String _tempIp1;
  String _tempIp2;
  String _tempIp3;
  String _tempIp4;

  // Callbacks estáticos para compatibilidade com o servidor Web do ESP32
  static void handleRootCallback();
  static void handleSaveCallback();
  static void handleInfoApiCallback();
  static void handleInfoPageCallback(); 

  // Métodos internos de processamento de rotas HTTP
  void handleRoot();
  void handleInfoPage();
  void handleSave();
  void handleInfoApi(); 
  bool tentarConectar(const String& ssid, const String& senha);

public:
  WifiConfig(Display& displayRef);
  bool conectar();
  void iniciarPortal();
  void processarPortal();
};

#endif
