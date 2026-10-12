#include "WifiConfig.h"
#include "CommandHandler.h"

#include "WebPageGzip.h"
#include "ClimaManager.h"
#include "SdManager.h"

static String jsonTexto(const String& texto) {
  String out = "\"";
  for (size_t i = 0; i < texto.length(); ++i) {
    unsigned char c = texto[i];
    if (c == '"' || c == '\\') { out += '\\'; out += char(c); }
    else if (c < 32) { char codigo[7]; snprintf(codigo, sizeof(codigo), "\\u%04x", c); out += codigo; }
    else out += char(c);
  }
  return out + "\"";
}

WifiConfig* WifiConfig::_instance = nullptr;

WifiConfig::WifiConfig(Display& displayRef) 
  : _server(80), _display(displayRef), _dadosProntosParaSalvar(false) {
  _instance = this;
}

void WifiConfig::handleRootCallback() { if (_instance) _instance->handleRoot(); }
void WifiConfig::handleSaveCallback() { if (_instance) _instance->handleSave(); }
void WifiConfig::handleInfoApiCallback() { if (_instance) _instance->handleInfoApi(); }
void WifiConfig::handleInfoPageCallback() { if (_instance) _instance->handleInfoPage(); }
void WifiConfig::handleLogCallback() { if (_instance) _instance->handleLog(); }

void WifiConfig::handleLog() {
  String conteudo;
  int status = 200;
  {
    SdManager::Acesso acesso;
    if (!acesso.pronto()) { status = 503; conteudo = "Cartao SD indisponivel."; }
    else {
      File arquivo = SD.open("/log.txt", FILE_READ);
      if (!arquivo) { status = 404; conteudo = "Arquivo de log ainda nao disponivel."; }
      else {
        size_t tamanho = arquivo.size();
        bool parcial = tamanho > 16384;
        if (parcial) arquivo.seek(tamanho - 16384);
        // Comeca numa linha completa quando apenas a cauda do arquivo e exibida.
        if (parcial) while (arquivo.available() && arquivo.read() != '\n') {}
        conteudo.reserve(16384);
        char bloco[513];
        while (arquivo.available() && conteudo.length() < 16384) {
          size_t quantidade = arquivo.read(reinterpret_cast<uint8_t*>(bloco), min(size_t(512), size_t(16384 - conteudo.length())));
          if (!quantidade) break;
          bloco[quantidade] = 0;
          conteudo += bloco;
        }
        arquivo.close();
      }
    }
  }
  _server.sendHeader("Cache-Control", "no-store");
  _server.send(status, "text/plain; charset=utf-8", conteudo);
}

void WifiConfig::handleRoot() { 
  _server.sendHeader("Content-Encoding", "gzip");
  _server.sendHeader("Cache-Control", "no-store");
  _server.send_P(200, "text/html; charset=utf-8", reinterpret_cast<const char*>(htmlPageGzip), sizeof(htmlPageGzip));
}

void WifiConfig::handleInfoPage() {
  handleRoot();
}

void WifiConfig::handleInfoApi() {
  extern UdpComm udp;
  bool conectado = WiFi.status() == WL_CONNECTED;
  String json;
  json.reserve(2600);
  json = "{";
  auto campo = [&](const String& nome, const String& valor) {
    json += jsonTexto(nome) + ":" + jsonTexto(valor) + ",";
  };
  campo("hostname", "ESP32_CENTRAL");
  campo("firmware", CommandHandler::obterVersaoAutomatica());
  campo("modo_rede", conectado ? "Wi-Fi conectado" : "Portal de configuracao / sem Wi-Fi");
  campo("rede_ssid", conectado ? WiFi.SSID() : "ESP32_CENTRAL_CONFIG");
  campo("ip_local", conectado ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
  campo("sinal_rssi", conectado ? String(WiFi.RSSI()) + " dBm" : "Sem sinal");
  campo("ram_livre", String(ESP.getFreeHeap() / 1024) + " KB");
  campo("flash_livre", String(float(ESP.getFreeSketchSpace()) / (1024 * 1024), 1) + " MB");
  campo("motivo_reset", CommandHandler::obterMotivoReset());
  campo("mac", WiFi.macAddress());
  campo("gateway", conectado ? WiFi.gatewayIP().toString() : "--");
  campo("dns", conectado ? WiFi.dnsIP().toString() : "--");
  campo("build", String(__DATE__) + " " + __TIME__);
  float temperaturaCpu = temperatureRead();
  campo("temp_cpu", isfinite(temperaturaCpu) ? String(temperaturaCpu, 1) + " C" : "Indisponivel");
  campo("cpu_mhz", String(ESP.getCpuFreqMHz()) + " MHz");
  json += "\"wifi_conectado\":" + String(conectado ? "true" : "false") + ",";
  json += "\"rssi_dbm\":" + String(conectado ? WiFi.RSSI() : -127) + ",";
  json += "\"uptime_segundos\":" + String(millis() / 1000) + ",";
  _prefs.begin("wifi", true);
  for (int i = 0; i < MAX_REDES; ++i)
    campo("_form_ssid" + String(i), _prefs.getString(("s" + String(i)).c_str(), ""));
  for (int i = 1; i <= 4; ++i) {
    String ip = udp.obterIpEscravo(i);
    if (!ip.length()) {
      const char* padroes[] = {"192.168.0.120", "192.168.0.125", "192.168.0.130", "192.168.0.135"};
      ip = _prefs.getString(("ip" + String(i)).c_str(), padroes[i - 1]);
    }
    campo("_form_ip" + String(i), ip);
    json += "\"esp" + String(i) + "_online\":" + String(LayoutDatabase::statusEscravos[i] ? "true" : "false") + ",";
  }
  _prefs.end();
  auto clima = ClimaManager::snapshot();
  json += "\"clima\":{";
  json += "\"sincronizado\":" + String(clima.sincronizado ? "true" : "false") + ",";
  json += "\"previsao_valida\":" + String(clima.previsaoValida ? "true" : "false") + ",";
  json += "\"ultima_falhou\":" + String(clima.ultimaFalhou ? "true" : "false") + ",";
  json += "\"temperatura\":" + String(clima.temperatura, 1) + ",";
  json += "\"umidade\":" + String(clima.umidade) + ",";
  json += "\"minima\":" + String(clima.minima, 1) + ",";
  json += "\"maxima\":" + String(clima.maxima, 1) + ",";
  json += "\"chuva\":" + String(clima.chuva) + ",";
  json += "\"idade_s\":" + String((millis() - clima.ultimaAtualizacao) / 1000) + ",";
  json += "\"data\":" + jsonTexto(String(clima.data)) + ",";
  json += "\"condicao\":" + jsonTexto(ClimaManager::obterTextoCondicao()) + "}}";
  _server.sendHeader("Cache-Control", "no-store");
  _server.send(200, "application/json; charset=utf-8", json);
}
void WifiConfig::handleSave() {
  if (_dadosProntosParaSalvar) {
    _server.send(409, "text/plain; charset=utf-8", "Configuracoes ja estao sendo aplicadas.");
    return;
  }
  for (int i = 1; i <= 4; ++i) {
    IPAddress ip;
    String valor = _server.arg("ip" + String(i)); valor.trim();
    if (!ip.fromString(valor)) {
      _server.send(400, "text/plain; charset=utf-8", "Endereco IPv4 invalido para ESP " + String(i));
      return;
    }
  }
  for (int i = 0; i < MAX_REDES; ++i) {
    if (_server.arg("ssid" + String(i)).length() > 32 || _server.arg("senha" + String(i)).length() > 63) {
      _server.send(400, "text/plain; charset=utf-8", "Nome ou senha da rede excede o tamanho permitido.");
      return;
    }
  }
  for (int i = 0; i < MAX_REDES; i++) {
    _tempSSIDs[i]  = _server.arg("ssid" + String(i));
    _tempSenhas[i] = _server.arg("senha" + String(i));
    _tempSSIDs[i].trim();
  }
  _tempIp1   = _server.arg("ip1");
  _tempIp2   = _server.arg("ip2");
  _tempIp3   = _server.arg("ip3"); 
  _tempIp4   = _server.arg("ip4"); 
  _tempIp1.trim(); _tempIp2.trim(); _tempIp3.trim(); _tempIp4.trim();
  
  _server.send(200, "text/html", "<h2>Configuracoes salvas! A Central esta aplicando os dados e reiniciando...</h2>");
  _dadosProntosParaSalvar = true;
}

bool WifiConfig::tentarConectar(const String& ssid, const String& senha) {
  WiFi.disconnect();
  WiFi.begin(ssid.c_str(), senha.c_str());
  _display.mostrarMensagemCentral(("Conectando: " + ssid).c_str());

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    tentativas++;
  }
  return WiFi.status() == WL_CONNECTED;
}

bool WifiConfig::conectar() {
  _prefs.begin("wifi", false);
  // Migra a rede unica antiga para o slot 0
  if (_prefs.getString("s0", "") == "" && _prefs.getString("ssid", "") != "") {
    _prefs.putString("s0", _prefs.getString("ssid", ""));
    _prefs.putString("p0", _prefs.getString("senha", ""));
  }
  String ssids[MAX_REDES], senhas[MAX_REDES];
  bool temRede = false;
  for (int i = 0; i < MAX_REDES; i++) {
    ssids[i]  = _prefs.getString(("s" + String(i)).c_str(), "");
    senhas[i] = _prefs.getString(("p" + String(i)).c_str(), "");
    if (ssids[i] != "") temRede = true;
  }
  int ultimo = _prefs.getInt("idx", -1);
  _prefs.end();

  if (!temRede) return false;

  WiFi.mode(WIFI_AP_STA);
  _display.mostrarMensagemCentral("Buscando redes...");
  int n = WiFi.scanNetworks();

  // Lista circular: comeca na rede seguinte a ultima usada
  bool conectado = false;
  for (int k = 1; k <= MAX_REDES && !conectado; k++) {
    int i = (ultimo + k + MAX_REDES) % MAX_REDES;
    if (ssids[i] == "") continue;

    bool visivel = false;
    for (int j = 0; j < n; j++) {
      if (WiFi.SSID(j) == ssids[i]) { visivel = true; break; }
    }
    if (!visivel) continue;

    if (tentarConectar(ssids[i], senhas[i])) {
      _prefs.begin("wifi", false);
      _prefs.putInt("idx", i);
      _prefs.end();
      conectado = true;
    }
  }
  WiFi.scanDelete();

  if (conectado) {
      _server.on("/", HTTP_GET, WifiConfig::handleRootCallback);
      _server.on("/info", HTTP_GET, WifiConfig::handleInfoPageCallback);
      _server.on("/salvar", HTTP_POST, WifiConfig::handleSaveCallback);
      _server.on("/api/info", HTTP_GET, WifiConfig::handleInfoApiCallback);
      _server.on("/api/log", HTTP_GET, WifiConfig::handleLogCallback);
      _server.begin();
      return true;
  }
  return false;
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
  _server.on("/info", HTTP_GET, WifiConfig::handleInfoPageCallback);
  _server.on("/salvar", HTTP_POST, WifiConfig::handleSaveCallback);
  _server.on("/api/info", HTTP_GET, WifiConfig::handleInfoApiCallback);
  _server.on("/api/log", HTTP_GET, WifiConfig::handleLogCallback);
  _server.begin();
}

void WifiConfig::processarPortal() {
  _server.handleClient();
  
  if (_dadosProntosParaSalvar) {
    _dadosProntosParaSalvar = false;
    
    _display.mostrarMensagemCentral("Gravando dados...", TFT_YELLOW);
    delay(500);
    
    _prefs.begin("wifi", false);
    for (int i = 0; i < MAX_REDES; i++) {
      String chaveS = "s" + String(i), chaveP = "p" + String(i);
      if (_tempSSIDs[i] == "") {
        _prefs.remove(chaveS.c_str());
        _prefs.remove(chaveP.c_str());
      } else {
        // Senha vazia com o mesmo SSID mantem a senha ja salva
        bool mesmaRede = (_prefs.getString(chaveS.c_str(), "") == _tempSSIDs[i]);
        if (_tempSenhas[i] != "" || !mesmaRede) _prefs.putString(chaveP.c_str(), _tempSenhas[i]);
        _prefs.putString(chaveS.c_str(), _tempSSIDs[i]);
      }
    }
    _prefs.remove("ssid");
    _prefs.remove("senha");
    _prefs.putInt("idx", -1);
    _prefs.putString("ip1", _tempIp1);
    _prefs.putString("ip2", _tempIp2);
    _prefs.putString("ip3", _tempIp3); 
    _prefs.putString("ip4", _tempIp4);
    _prefs.end();
    
    _display.mostrarMensagemCentral("Reiniciando...", TFT_GREEN);
    delay(1500);
    ESP.restart();
  }
}
