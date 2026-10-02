#include "WifiConfig.h"

// HTML com os campos corretos ajustados para o CSS responsivo
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Configuração Central WiFi</title>
<style>
  body { font-family: Arial, sans-serif; margin: 30px; background-color: #f4f4f9; text-align: center; }
  .container { background: white; max-width: 320px; margin: auto; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); text-align: left; }
  h2 { text-align: center; color: #333; }
  label { font-weight: bold; display: block; margin-top: 10px; color: #555; }
  input { width: 100%; padding: 8px; margin: 5px 0 15px 0; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; }
  input[type="submit"] { background: #007bff; color: white; border: none; cursor: pointer; font-size: 16px; font-weight: bold; margin-top: 10px; }
  input[type="submit"]:hover { background: #0056b3; }
</style>
</head>
<body>
<div class="container">
  <h2>Configuração Central</h2>
  <form action="/salvar" method="POST">
    <label>SSID do Wi-Fi:</label>
    <input type="text" name="ssid" placeholder="Nome da rede" required>
    <label>Senha do Wi-Fi:</label>
    <input type="password" name="senha" placeholder="Senha da rede">
    <label>IP do Escravo 1:</label>
    <input type="text" name="ip1" value="192.168.0.120" required>
    <label>IP do Escravo 2:</label>
    <input type="text" name="ip2" value="192.168.0.125" required>
    <input type="submit" value="Salvar Configurações">
  </form>
</div>
</body>
</html>
)rawliteral";

WifiConfig* WifiConfig::_instance = nullptr;

WifiConfig::WifiConfig(Display& displayRef) 
  : _server(80), _display(displayRef), _dadosProntosParaSalvar(false) {
  _instance = this;
}

void WifiConfig::handleRootCallback() { if (_instance) _instance->handleRoot(); }
void WifiConfig::handleSaveCallback() { if (_instance) _instance->handleSave(); }

void WifiConfig::handleRoot() { 
  _server.send(200, "text/html", htmlPage); 
}
  
void WifiConfig::handleSave() {
  // Captura os dados da requisição HTTP de forma rápida e segura
  _tempSSID  = _server.arg("ssid");
  _tempSenha = _server.arg("senha");
  _tempIp1   = _server.arg("ip1");
  _tempIp2   = _server.arg("ip2");
  
  // Responde ao navegador imediatamente antes de desligar o Wi-Fi
  _server.send(200, "text/html", "<h2>Configuracoes recebidas! A Central esta processando e reiniciando...</h2>");
  
  // Ativa a flag para o loop principal assumir a gravação física na Flash
  _dadosProntosParaSalvar = true;
}

bool WifiConfig::conectar() {
  _prefs.begin("wifi", true);
  String ssid = _prefs.getString("ssid", "");
  String password = _prefs.getString("senha", "");
  _prefs.end();

  if (ssid == "") return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  _display.mostrarMensagemCentral("Conectando ao Wi-Fi...");

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    tentativas++;
  }
  return WiFi.status() == WL_CONNECTED;
}

void WifiConfig::iniciarPortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32_CENTRAL_CONFIG");

  TFT_eSPI& tft = _display.getTftDriver();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setTextDatum(TL_DATUM);
  tft.drawString("MODO PORTAL ATIVO", 10, 30);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
  tft.drawString("Rede: ESP32_CENTRAL_CONFIG", 10, 70);
  tft.drawString("Acesse o IP: 192.168.4.1", 10, 100);

  _server.on("/", HTTP_GET, WifiConfig::handleRootCallback);
  _server.on("/salvar", HTTP_POST, WifiConfig::handleSaveCallback);
  _server.begin();
}

void WifiConfig::processarPortal() {
  if (WiFi.status() != WL_CONNECTED) {
    _server.handleClient();
    
    // Executa a gravação de forma síncrona e segura fora do contexto da interrupção HTTP
    if (_dadosProntosParaSalvar) {
      _dadosProntosParaSalvar = false;
      
      // Agora o barramento SPI da tela está livre e seguro para ser atualizado!
      _display.mostrarMensagemCentral("Gravando dados...", TFT_YELLOW);
      delay(500);
      
      _prefs.begin("wifi", false);
      _prefs.putString("ssid", _tempSSID);
      _prefs.putString("senha", _tempSenha);
      _prefs.putString("ip1", _tempIp1);
      _prefs.putString("ip2", _tempIp2);
      _prefs.end();
      
      _display.mostrarMensagemCentral("Reiniciando...", TFT_GREEN);
      delay(1500);
      ESP.restart();
    }
  }
}
