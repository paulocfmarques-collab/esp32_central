#include "WifiConfig.h"
#include "CommandHandler.h"

// Interface gráfica avançada - PARTE 1 (CSS e Estrutura Base)
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Dashboard Central UDP</title>
<style>
  :root {
    --bg-color: #0f172a;
    --card-bg: #1e293b;
    --primary: #3b82f6;
    --success: #10b981;
    --text-main: #f8fafc;
    --text-muted: #94a3b8;
    --border: #334155;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, sans-serif; }
  body { background-color: var(--bg-color); color: var(--text-main); padding: 20px; display: flex; flex-direction: column; align-items: center; }
  .wrapper { width: 100%; max-width: 700px; }
  header { text-align: center; margin-bottom: 25px; }
  header h1 { font-size: 24px; color: #f1f5f9; }
  header p { color: var(--text-muted); font-size: 14px; margin-top: 5px; }
  
  /* Sistema de Abas Navegáveis */
  .tabs { display: flex; background: var(--card-bg); border-radius: 8px; padding: 4px; margin-bottom: 20px; border: 1px solid var(--border); }
  .tab-btn { flex: 1; padding: 10px; background: transparent; border: none; color: var(--text-muted); font-weight: 600; cursor: pointer; border-radius: 6px; transition: all 0.2s; }
  .tab-btn.active { background: var(--primary); color: var(--text-main); }
  .tab-content { display: none; }
  .tab-content.active { display: block; }

  /* Estilos do Painel de Diagnóstico */
  .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin-bottom: 20px; }
  .card { background: var(--card-bg); padding: 15px; border-radius: 8px; border: 1px solid var(--border); display: flex; flex-direction: column; }
  .card .label { font-size: 11px; text-transform: uppercase; color: var(--text-muted); font-weight: 700; letter-spacing: 0.5px; }
  .card .value { font-size: 18px; font-weight: 600; color: #fff; margin-top: 5px; word-break: break-all; }
  .refresh-area { text-align: center; margin-top: 10px; }
  .btn { background: var(--primary); color: white; border: none; padding: 12px 20px; font-size: 15px; font-weight: bold; border-radius: 6px; cursor: pointer; width: 100%; transition: opacity 0.2s; }
  .btn:hover { opacity: 0.9; }
  .btn-success { background: var(--success); }

  /* Estilos do Form de Configurações */
  form { background: var(--card-bg); padding: 20px; border-radius: 8px; border: 1px solid var(--border); }
  .form-group { margin-bottom: 15px; }
  label { display: block; font-size: 13px; font-weight: 600; margin-bottom: 6px; color: #cbd5e1; }
  input { width: 100%; padding: 10px; background: #0f172a; border: 1px solid var(--border); border-radius: 6px; color: white; font-size: 14px; }
  input:focus { border-color: var(--primary); outline: none; }
</style>
</head>
<body>

<div class="wrapper">
  <header>
    <h1>Central de Gerenciamento UDP</h1>
    <p>Painel de Controle e Monitoramento do Mestre (redes salvas: ate 5, rotacao circular)</p>
  </header>

  <div class="tabs">
    <button class="tab-btn active" onclick="switchTab('dashboard')">Status do Sistema</button>
    <button class="tab-btn" onclick="switchTab('config')">Configurar Dispositivos</button>
  </div>

  <!-- ABA 1: DASHBOARD DINÂMICO -->
  <div id="dashboard" class="tab-content active">
    <div class="grid" id="info-grid">
      <div class="card"><div class="label">Carregando dados...</div><div class="value">---</div></div>
    </div>
    <div class="refresh-area">
      <button class="btn" onclick="atualizarDadosInfo()">Atualizar Diagnóstico</button>
    </div>
  </div>
  <!-- ABA 2: CONFIGURAÇÃO DE IPS DOS ESCRAVOS -->
  <div id="config" class="tab-content">
    <form action="/salvar" method="POST">
      <div class="grid" style="grid-template-columns: repeat(auto-fit, minmax(260px, 1fr)); gap: 10px;">
        <div class="form-group"><label>Rede 1 - SSID:</label><input type="text" name="ssid0" id="form-ssid0"></div>
        <div class="form-group"><label>Rede 1 - Senha:</label><input type="password" name="senha0" placeholder="(vazio = manter)"></div>
        <div class="form-group"><label>Rede 2 - SSID:</label><input type="text" name="ssid1" id="form-ssid1"></div>
        <div class="form-group"><label>Rede 2 - Senha:</label><input type="password" name="senha1" placeholder="(vazio = manter)"></div>
        <div class="form-group"><label>Rede 3 - SSID:</label><input type="text" name="ssid2" id="form-ssid2"></div>
        <div class="form-group"><label>Rede 3 - Senha:</label><input type="password" name="senha2" placeholder="(vazio = manter)"></div>
        <div class="form-group"><label>Rede 4 - SSID:</label><input type="text" name="ssid3" id="form-ssid3"></div>
        <div class="form-group"><label>Rede 4 - Senha:</label><input type="password" name="senha3" placeholder="(vazio = manter)"></div>
        <div class="form-group"><label>Rede 5 - SSID:</label><input type="text" name="ssid4" id="form-ssid4"></div>
        <div class="form-group"><label>Rede 5 - Senha:</label><input type="password" name="senha4" placeholder="(vazio = manter)"></div>
      </div>
      <div class="grid" style="grid-template-columns: repeat(auto-fit, minmax(260px, 1fr)); gap: 10px;">
        <div class="form-group"><label>Endereço IP - Escravo 1:</label><input type="text" name="ip1" id="form-ip1" required></div>
        <div class="form-group"><label>Endereço IP - Escravo 2:</label><input type="text" name="ip2" id="form-ip2" required></div>
        <div class="form-group"><label>Endereço IP - Escravo 3:</label><input type="text" name="ip3" id="form-ip3" required></div>
        <div class="form-group"><label>Endereço IP - Escravo 4:</label><input type="text" name="ip4" id="form-ip4" required></div>
      </div>
      <button type="submit" class="btn btn-success">Salvar e Reiniciar Central</button>
    </form>
  </div>
</div>

<script>
  function switchTab(tabId) {
    document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
    document.querySelectorAll('.tab-btn').forEach(el => el.classList.remove('active'));
    document.getElementById(tabId).classList.add('active');
    event.target.classList.add('active');
    if(tabId === 'dashboard') atualizarDadosInfo();
  }

  function atualizarDadosInfo() {
    fetch('/api/info')
      .then(response => response.json())
      .then(data => {
        const grid = document.getElementById('info-grid');
        grid.innerHTML = '';
        
        Object.keys(data).forEach(key => {
          if(key.startsWith('_form_')) return; 
          
          const card = document.createElement('div');
          card.className = 'card';
          let labelText = key.replace('_', ' ').toUpperCase();
          card.innerHTML = `<div class="label">${labelText}</div><div class="value">${data[key]}</div>`;
          grid.appendChild(card);
        });

        for(let i = 0; i < 5; i++) { const el = document.getElementById('form-ssid' + i); if(el && data['_form_ssid' + i] !== undefined) el.value = data['_form_ssid' + i]; }
        if(data._form_ip1) document.getElementById('form-ip1').value = data._form_ip1;
        if(data._form_ip2) document.getElementById('form-ip2').value = data._form_ip2;
        if(data._form_ip3) document.getElementById('form-ip3').value = data._form_ip3;
        if(data._form_ip4) document.getElementById('form-ip4').value = data._form_ip4;
      })
      .catch(err => {
        document.getElementById('info-grid').innerHTML = '<div class="card" style="grid-column: 1/-1; text-align:center;"><div class="label" style="color:#ef4444;">Erro de Comunicação</div><div class="value">Não foi possível acessar a API interna.</div></div>';
      });
  }

  window.onload = atualizarDadosInfo;
</script>
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
void WifiConfig::handleInfoApiCallback() { if (_instance) _instance->handleInfoApi(); }
void WifiConfig::handleInfoPageCallback() { if (_instance) _instance->handleInfoPage(); }

void WifiConfig::handleRoot() { 
  _server.send(200, "text/html", htmlPage); 
}

void WifiConfig::handleInfoPage() {
  _server.send(200, "text/html", htmlPage); 
}

void WifiConfig::handleInfoApi() {
    extern UdpComm udp;
    String json = "{";
    
    json += "\"hostname\":\"ESP32_MESTRE\",";
    json += "\"firmware\":\"1.0.0_MSTR\",";
    json += "\"rede_ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"ip_local\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"sinal_rssi\":\"" + String(WiFi.RSSI()) + " dBm\",";
    json += "\"ram_livre\":\"" + String(ESP.getFreeHeap() / 1024) + " KB\",";
    json += "\"flash_livre\":\"" + String((float)ESP.getFreeSketchSpace() / (1024.0 * 1024.0), 1) + " MB\",";
    json += "\"uptime_sistema\":\"" + String(millis() / 1000) + " segundos\",";
    json += "\"motivo_reset\":\"" + CommandHandler::obterMotivoReset() + "\",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\",";
    json += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\",";
    json += "\"dns\":\"" + WiFi.dnsIP(0).toString() + "\",";
    json += "\"build\":\"" + String(__DATE__) + " " + String(__TIME__) + "\",";
    json += "\"temp_cpu\":\"" + String(temperatureRead(), 1) + " C\",";
    json += "\"cpu_mhz\":\"" + String(ESP.getCpuFreqMHz()) + " MHz\",";
    {
      _prefs.begin("wifi", true);
      for (int i = 0; i < MAX_REDES; i++) {
        json += "\"_form_ssid" + String(i) + "\":\"" + _prefs.getString(("s" + String(i)).c_str(), "") + "\",";
      }
      _prefs.end();
    }
    
    // Injeta os IPs em cache na RAM para preenchimento automático das caixas no JavaScript
    json += "\"_form_ip1\":\"" + udp.obterIpEscravo(1) + "\",";
    json += "\"_form_ip2\":\"" + udp.obterIpEscravo(2) + "\",";
    json += "\"_form_ip3\":\"" + udp.obterIpEscravo(3) + "\",";
    json += "\"_form_ip4\":\"" + udp.obterIpEscravo(4) + "\"";
    json += "}";
    
    _server.send(200, "application/json", json);
}
  
void WifiConfig::handleSave() {
  for (int i = 0; i < MAX_REDES; i++) {
    _tempSSIDs[i]  = _server.arg("ssid" + String(i));
    _tempSenhas[i] = _server.arg("senha" + String(i));
    _tempSSIDs[i].trim();
  }
  _tempIp1   = _server.arg("ip1");
  _tempIp2   = _server.arg("ip2");
  _tempIp3   = _server.arg("ip3"); 
  _tempIp4   = _server.arg("ip4"); 
  
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
