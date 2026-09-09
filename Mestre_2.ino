#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Preferences.h>

#define XPT_CLK   25
#define XPT_MISO  39
#define XPT_MOSI  32
#define XPT_CS    33
#define SCREEN_W  320
#define SCREEN_H  240
#define LED       4

const char* ESP1 = "192.168.0.120";
const char* ESP2 = "192.168.0.125";
const int udpPort    = 4210;

bool ESP1Selecionado = true; 
bool blinkAtivo = false;
bool estadoLed = false;
unsigned long ultimoToggle = 0;
unsigned long intervaloBlink = 500; 
bool telaPrincipalAtiva = true; 

SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(XPT_CS);
TFT_eSPI tft = TFT_eSPI();
WiFiUDP udp;
WebServer server(80);
Preferences prefs;

struct Botao {
  int x, y, w, h;
  const char* label;
  const char* comando;
  uint16_t cor;
};

// Matriz otimizada com 12 botões compactos distribuídos em 3 colunas e 4 linhas
const int TOTAL_BOTOES = 12;
Botao actionButtons[TOTAL_BOTOES] = {
  {10,  95,  94, 26, "LED ON",    "LED_ON",        TFT_GREEN},
  {113, 95,  94, 26, "LED OFF",   "LED_OFF",       TFT_RED},
  {216, 95,  94, 26, "BLINK 1s",  "LED_BLINK:1000",TFT_PURPLE},
  
  {10,  126, 94, 26, "VER TEMP",  "TEMP",          TFT_BLUE},
  {113, 126, 94, 26, "INFO CPU",  "CPU",           TFT_ORANGE},
  {216, 126, 94, 26, "RAM FREE",  "RAM",           0x51D0}, // Azul acinzentado
  
  {10,  157, 94, 26, "FLASH INF", "FLASH",         0x91a4}, // Marrom claro
  {113, 157, 94, 26, "RST MOTIV", "INIT",          0x7BEF}, // Cinza
  {216, 157, 94, 26, "UPTIME",    "UPTIME",        0x03E0}, // Verde escuro
  
  {10,  188, 94, 26, "END MAC",   "MAC",           0xB1DF}, // Roxo claro
  {113, 188, 94, 26, "REDE INFO", "NET_INFO",     0x05FF}, // Ciano
  {216, 188, 94, 26, "RST WIFI",  "RESET_WIFI",    0xA000}  // Vermelho escuro
};

int sel1X = 15,  sel1Y = 48, sel1W = 135, sel1H = 40;
int sel2X = 170, sel2Y = 48, sel2W = 135, sel2H = 40;

unsigned long ultimoTouch = 0;
String respostaAcumulada = "";

const char* htmlPage PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Configuração Central WiFi</title>
<style>
  body { font-family: Arial, sans-serif; margin: 40px; background-color: #f4f4f9; text-align: center; }
  .container { background: white; max-width: 300px; margin: auto; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
  input { width: 100%; padding: 8px; margin: 10px 0; box-sizing: border-box; }
  input[type="submit"] { background: #007bff; color: white; border: none; cursor: pointer; }
</style>
</head>
<body>
<div class="container">
  <h2>Configuração Central - WiFi</h2>
  <form action="/salvar" method="POST">
    <label>SSID:</label><input type="text" name="ssid" placeholder="Nome da rede" required>
    <label>Senha:</label><input type="password" name="senha" placeholder="Senha da rede">
    <input type="submit" value="Salvar">
  </form>
</div>
</body>
</html>
)rawliteral";

void desenharInterface();
void desenharTelaResposta(String conteudo);
void processarClique(int x, int y);
void enviarComandoUDP(const char* cmd);
void verificarMensagensUDP();
void executa_comando_local(String cmd);
void iniciarPortal();
void salvarWifi();
bool conectarWifi();
void zerarConfiguracoes();

void salvarWifi() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
  tft.drawString("Salvando e Reiniciando...", SCREEN_W/2, SCREEN_H/2);
  String novoSSID = server.arg("ssid");
  String novaSenha = server.arg("senha");
  prefs.begin("wifi", false);
  prefs.putString("ssid", novoSSID);
  prefs.putString("senha", novaSenha);
  prefs.end();
  server.send(200, "text/html", "<h2>Configuracao salva! A Central esta reiniciando...</h2>");
  delay(2000);
  ESP.restart();
}

void iniciarPortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32_CENTRAL_CONFIG");
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setTextDatum(TL_DATUM);
  tft.drawString("MODO PORTAL ATIVO", 10, 30);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
  tft.drawString("Rede: ESP32_CENTRAL_CONFIG", 10, 70);
  tft.drawString("Acesse o IP: 192.168.4.1", 10, 100);
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", htmlPage); });
  server.on("/salvar", HTTP_POST, salvarWifi);
  server.begin();
}

bool conectarWifi() {
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("senha", "");
  prefs.end();
  if (ssid == "") return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
  tft.drawString("Conectando ao Wi-Fi...", SCREEN_W/2, SCREEN_H/2 - 20);
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) { delay(500); tentativas++; }
  return WiFi.status() == WL_CONNECTED;
}

void zerarConfiguracoes() {
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.print("Central: WiFi zerado. Reiniciando...\n");
  udp.endPacket();
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT); digitalWrite(LED, HIGH); 
  pinMode(21, OUTPUT); digitalWrite(21, HIGH); 
  
  tft.init(); tft.setRotation(1); tft.invertDisplay(true); tft.fillScreen(TFT_BLACK);
  touchSPI.begin(XPT_CLK, XPT_MISO, XPT_MOSI, XPT_CS);
  ts.begin(touchSPI); ts.setRotation(1);

  if (conectarWifi()) {
    server.stop(); WiFi.softAPdisconnect(true);
    udp.begin(udpPort);
    desenharInterface();
  } else {
    iniciarPortal();
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    server.handleClient();
  } else {
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      unsigned long agora = millis();
      if (p.z > 150 && p.x > 0 && (agora - ultimoTouch > 400)) {
        ultimoTouch = agora;
        int x = map(p.x, 300, 3900, 0, SCREEN_W);
        int y = map(p.y, 200, 3700, 0, SCREEN_H);
        
        if (!telaPrincipalAtiva) {
          telaPrincipalAtiva = true;
          desenharInterface();
        } else {
          processarClique(x, y);
        }
      }
    }
    
    verificarMensagensUDP();
    
    if (blinkAtivo) {
      unsigned long atual = millis();
      if (atual - ultimoToggle >= intervaloBlink) {
        ultimoToggle = atual;
        estadoLed = !estadoLed;
        digitalWrite(LED, estadoLed ? LOW : HIGH);
      }
    }
  }
}

void desenharInterface() {
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, SCREEN_W, 38, 0x2103);
  tft.setTextColor(TFT_GOLD); tft.setTextSize(2); tft.setTextDatum(TL_DATUM);
  tft.drawString(" CENTRAL HIBRIDA TOTAL", 10, 10);

  tft.setTextSize(1); tft.setTextDatum(MC_DATUM);
  tft.fillRoundRect(sel1X, sel1Y, sel1W, sel1H, 4, ESP1Selecionado ? 0x0410 : TFT_DARKGREY);
  tft.drawRoundRect(sel1X, sel1Y, sel1W, sel1H, 4, ESP1Selecionado ? TFT_CYAN : TFT_WHITE);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("ESP 1 (120)", sel1X + (sel1W/2), sel1Y + (sel1H/2));

  tft.fillRoundRect(sel2X, sel2Y, sel2W, sel2H, 4, !ESP1Selecionado ? 0x0410 : TFT_DARKGREY);
  tft.drawRoundRect(sel2X, sel2Y, sel2W, sel2H, 4, !ESP1Selecionado ? TFT_CYAN : TFT_WHITE);
  tft.drawString("ESP 2 (125)", sel2X + (sel2W/2), sel2Y + (sel2H/2));

  // Renderiza os 12 botões em escala compacta
  tft.setTextSize(1);
  for (int i = 0; i < TOTAL_BOTOES; i++) {
    tft.fillRoundRect(actionButtons[i].x, actionButtons[i].y, actionButtons[i].w, actionButtons[i].h, 3, actionButtons[i].cor);
    tft.drawRoundRect(actionButtons[i].x, actionButtons[i].y, actionButtons[i].w, actionButtons[i].h, 3, TFT_WHITE);
    tft.setTextColor((actionButtons[i].cor == TFT_GREEN || actionButtons[i].cor == TFT_ORANGE || actionButtons[i].cor == TFT_YELLOW) ? TFT_BLACK : TFT_WHITE);
    tft.drawString(actionButtons[i].label, actionButtons[i].x + (actionButtons[i].w / 2), actionButtons[i].y + (actionButtons[i].h / 2));
  }
}

void desenharTelaResposta(String conteudo) {
  telaPrincipalAtiva = false;
  tft.fillScreen(0x0821); 
  
  tft.fillRect(0, 0, SCREEN_W, 35, TFT_DARKGREY);
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setTextDatum(TL_DATUM);
  tft.drawString(" TELA DE RESULTADOS", 10, 8);

  tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.setTextDatum(TL_DATUM);
  
  int yPos = 48;
  int startIdx = 0;
  while(startIdx < conteudo.length()) {
    int endIdx = conteudo.indexOf('\n', startIdx);
    if(endIdx == -1) endIdx = conteudo.length();
    String linha = conteudo.substring(startIdx, endIdx);
    
    // Proteção para pular linhas vazias estáticas secundárias
    if (linha.length() > 0 || endIdx != conteudo.length()) {
      tft.drawString(linha, 15, yPos);
      yPos += 14;
    }
    startIdx = endIdx + 1;
    if(yPos > 210) break; // Trava para não estourar o rodapé físico
  }

  tft.fillRect(0, 222, SCREEN_W, 18, TFT_BLACK);
  tft.setTextColor(TFT_GREEN); tft.setTextDatum(MC_DATUM);
  tft.drawString("[ Toque na tela para fechar ]", SCREEN_W / 2, 231);
}

void processarClique(int x, int y) {
  if (x >= sel1X && x <= (sel1X + sel1W) && y >= sel1Y && y <= (sel1Y + sel1H)) {
    if (!ESP1Selecionado) { ESP1Selecionado = true; desenharInterface(); }
    return;
  }
  if (x >= sel2X && x <= (sel2X + sel2W) && y >= sel2Y && y <= (sel2Y + sel2H)) {
    if (ESP1Selecionado) { ESP1Selecionado = false; desenharInterface(); }
    return;
  }

  for (int i = 0; i < TOTAL_BOTOES; i++) {
    if (x >= actionButtons[i].x && x <= (actionButtons[i].x + actionButtons[i].w) && y >= actionButtons[i].y && y <= (actionButtons[i].y + actionButtons[i].h)) {
      // Se o comando for executado localmente de forma direta no painel (Ex: RESET_WIFI)
      if (String(actionButtons[i].comando) == "RESET_WIFI") {
        zerarConfiguracoes();
      } else {
        enviarComandoUDP(actionButtons[i].comando);
      }
      break;
    }
  }
}

void enviarComandoUDP(const char* cmd) {
  IPAddress targetIP;
  const char* destinoStr = ESP1Selecionado ? ESP1 : ESP2;
  if(targetIP.fromString(destinoStr)) {
    udp.beginPacket(targetIP, udpPort);
    udp.print(cmd);
    udp.endPacket();
  }
}

void verificarMensagensUDP() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char packetBuffer[512]; 
    int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = '\0';
      String msg = String(packetBuffer);
      msg.trim();

      if (udp.remoteIP().toString() == String(ESP1) || udp.remoteIP().toString() == String(ESP2)) {
        desenharTelaResposta(msg);
      } else {
        executa_comando_local(msg);
      }
    }
  }
}

void executa_comando_local(String cmd) {
  String resp = "";
  
  if (cmd == "RESET_WIFI") { 
    zerarConfiguracoes(); 
  }
  else if (cmd == "LED_ON") {
    blinkAtivo = false; estadoLed = true; digitalWrite(LED, LOW);
    resp = "Central Local:\nLED foi ligado com sucesso.";
  }
  else if (cmd == "LED_OFF") {
    blinkAtivo = false; estadoLed = false; digitalWrite(LED, HIGH);
    resp = "Central Local:\nLED foi desligado.";
  }
  else if (cmd == "TEMP") {
    resp = "Central Local:\nCPU Temp: " + String(temperatureRead()) + " C";
  }
  else if (cmd == "CPU") {
    resp = "Central Local CPU:\nModelo: " + String(ESP.getChipModel()) + "\nRevisao: " + String(ESP.getChipRevision()) + "\nNucleos: " + String(ESP.getChipCores()) + "\nFreq: " + String(ESP.getCpuFreqMHz()) + " MHz";
  }
  else if (cmd == "RAM") {
    resp = "Central Local RAM:\nHeap livre: " + String(ESP.getFreeHeap()) + " bytes\nMenor heap: " + String(ESP.getMinFreeHeap()) + " bytes\nMaior bloco: " + String(ESP.getMaxAllocHeap()) + " bytes";
  }
  else if (cmd == "FLASH") {
    resp = "Central Local FLASH:\nFlash total: " + String(ESP.getFlashChipSize()) + " bytes\nVelocidade: " + String(ESP.getFlashChipSpeed()) + " Hz\nSketch size: " + String(ESP.getSketchSize()) + " bytes\nEspaco livre: " + String(ESP.getFreeSketchSpace()) + " bytes";
  }
  else if (cmd == "INIT") {
    resp = "Central Local:\nMotivo do reset: " + String(esp_reset_reason());
  }
  else if (cmd == "UPTIME") {
    resp = "Central Local:\nUptime: " + String(millis()) + " ms";
  }
  else if (cmd == "MAC") {
    resp = "Central Local:\nMAC: " + WiFi.macAddress();
  }
  else if (cmd == "NET_INFO") {
    resp = "Central Local NET:\nIP: " + WiFi.localIP().toString() + "\nGateway: " + WiFi.gatewayIP().toString() + "\nMascara: " + WiFi.subnetMask().toString() + "\nRSSI: " + String(WiFi.RSSI()) + " dBm\nSSID: " + String(WiFi.SSID());
  }
  else if (cmd.startsWith("LED_BLINK")) {
    int p = cmd.indexOf(':');
    if (p > 0) intervaloBlink = cmd.substring(p + 1).toInt();
    blinkAtivo = true;
    resp = "Central Local:\nBlink ativo (" + String(intervaloBlink) + " ms)";
  }
  else {
    resp = "Central Local:\nComando desconhecido ou invalido.";
  }

  // Devolve o feedback textual completo para o Computador de volta via pacote UDP
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.print(resp + "\n");
  udp.endPacket();

  // FIXA A RESPOSTA NA NOVA PÁGINA CHEIA DO DISPLAY DE 2.8"
  desenharTelaResposta(resp);
}
