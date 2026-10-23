#include "MetricsService.h"
#include "CommandHandler.h"
#include "ClimaManager.h"
#include "SdManager.h"
#include "UdpComm.h"
#include "MetricsChunkStream.h"
#include <esp_heap_caps.h>

extern UdpComm udp;
extern QueueHandle_t filaMensagens;
extern TaskHandle_t TaskCore0;

String MetricsService::diagnostico() {
    if (!instance) return "Servico de metricas ainda nao inicializado.\n";
    auto& service = *instance;
    const uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    String out = "DIAGNOSTICO DE METRICAS\nPorta: " + String(service.activePort);
    out += "\nExportacoes OK: " + String(service.exportsOk);
    out += "\nExportacoes com falha: " + String(service.exportsFailed);
    if (service.exportsOk || service.exportsFailed) {
        out += "\nUltima tentativa: " + String((millis() - service.lastExportAt) / 1000) + " s atras";
        out += "\nUltimo resultado: " + String(service.lastExportFailed ? "FALHA" : "OK");
        out += "\nDuracao: " + String(service.lastExportDurationMs) + " ms";
        out += "\nRAM 8 bits antes da tentativa: " + String(service.lastFree8) + " bytes";
        out += "\nMaior bloco antes da tentativa: " + String(service.lastLargest8) + " bytes";
    }
    out += "\nRAM 8 bits livre agora: " + String(heap_caps_get_free_size(caps)) + " bytes";
    out += "\nMaior bloco 8 bits agora: " + String(heap_caps_get_largest_free_block(caps)) + " bytes";
    out += "\nMatrix: " + String(central.protecaoTelaAtiva() ? "ATIVO" : "INATIVO");
    out += "\nTarefa de rede criada: " + String(TaskCore0 ? "SIM" : "NAO");
    out += "\nFila de comandos: " + String(filaMensagens ? uxQueueMessagesWaiting(filaMensagens) : 0) + "\n";
    return out;
}

bool MetricsService::validHost(const String& host) {
    if (host.isEmpty() || host.length() > 253) return false;
    unsigned length = 0;
    char previous = 0;
    for (size_t i = 0; i < host.length(); ++i) {
        char c = host[i];
        bool alnum = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        if (c == '.') {
            if (!length || previous == '-') return false;
            length = 0;
        } else if ((!alnum && c != '-') || (!length && c == '-') || ++length > 63) return false;
        previous = c;
    }
    return length && previous != '-';
}

bool MetricsService::parseNumber(const String& text, uint16_t minimum, uint16_t maximum, uint16_t& out) {
    if (text.isEmpty() || text.length() > 5) return false;
    uint32_t value = 0;
    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
        value = value * 10 + text[i] - '0';
    }
    if (value < minimum || value > maximum) return false;
    out = static_cast<uint16_t>(value);
    return true;
}

String MetricsService::escapeLabel(String value) {
    value.replace("\\", "\\\\"); value.replace("\"", "\\\"");
    value.replace("\n", "\\n"); value.replace("\r", "\\n");
    return value;
}
String MetricsService::escapeHtml(String value) {
    value.replace("&", "&amp;"); value.replace("<", "&lt;"); value.replace(">", "&gt;");
    value.replace("\"", "&quot;"); value.replace("'", "&#39;");
    return value;
}

void MetricsService::begin(WebServer& web) {
    instance = this;
    panel = &web;
    pinMode(0, INPUT_PULLUP);
    config.version = 1; config.port = 9120; config.interval = 15;
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    char id[18];
    snprintf(id, sizeof(id), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    device = id;
    imageBytes = ESP.getSketchSize();
    const esp_partition_t* partition = esp_ota_get_running_partition();
    partitionBytes = partition ? partition->size : 0;
    Preferences prefs;
    if (prefs.begin("telemetry", false)) {
        Settings saved{};
        size_t length = prefs.getBytesLength("settings");
        if (length == sizeof(saved) && prefs.getBytes("settings", &saved, sizeof(saved)) == sizeof(saved) &&
            saved.version == 1 && memchr(saved.host, 0, sizeof(saved.host)) &&
            validHost(String(saved.host)) && saved.port && saved.interval >= 5 && saved.interval <= 3600)
            config = saved;
        else if (length) Serial.println("[METRICS] Configuracao NVS invalida; usando padroes.");
        length = prefs.getBytesLength("boots");
        bool valid = length == 0;
        if (length == sizeof(boot))
            valid = prefs.getBytes("boots", &boot, sizeof(boot)) == sizeof(boot) && boot.version == 1;
        if (valid) {
            boot.version = 1;
            if (boot.boots < UINT32_MAX) ++boot.boots;
            esp_reset_reason_t reason = esp_reset_reason();
            if ((reason == ESP_RST_WDT || reason == ESP_RST_INT_WDT || reason == ESP_RST_TASK_WDT) &&
                boot.watchdogs < UINT32_MAX) ++boot.watchdogs;
            countersOk = prefs.putBytes("boots", &boot, sizeof(boot)) == sizeof(boot);
        }
        prefs.end();
    } else Serial.println("[METRICS] NVS indisponivel; usando porta 9120 e intervalo 15s.");
    if (!countersOk) Serial.println("[METRICS] Contadores persistentes indisponiveis.");
#if !configUSE_TRACE_FACILITY || !configGENERATE_RUN_TIME_STATS
    Serial.println("[METRICS] CPU indisponivel: SDK sem estatisticas de runtime.");
#endif
    WiFi.onEvent([this](WiFiEvent_t, WiFiEventInfo_t) { disconnects.fetch_add(1); },
                 ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    auto record = [this](WebServer& server, Middleware::Callback next) {
        int64_t startUs = esp_timer_get_time();
        bool handled = next();
        ++requests;
        if (server.responseCode() >= 400 || server.responseCode() <= 0) ++errors;
        responseMs = (esp_timer_get_time() - startUs) / 1000.0;
        return handled;
    };
    web.addMiddleware(record);
    exporter.addMiddleware(record);
    exporter.on("/metrics", HTTP_GET, [this]() { sendMetrics(exporter); });
    web.on("/metrics", HTTP_GET, [this]() {
        if (activePort == 80) sendMetrics(*panel);
        else panel->send(404, "text/plain; charset=utf-8", "Use a porta de metricas configurada.");
    });
    web.on("/prometheus", HTTP_GET, [this]() { show(); });
    web.on("/prometheus", HTTP_POST, [this]() { handleSave(); });
    web.on("/prometheus.yml", HTTP_GET, [this]() {
        panel->sendHeader("Cache-Control", "no-store");
        panel->sendHeader("Content-Disposition", "attachment; filename=prometheus.yml");
        panel->send(200, "text/yaml; charset=utf-8", yaml());
    });
}

bool MetricsService::resetCounters() {
    Preferences prefs;
    if (!prefs.begin("telemetry", false)) return false;
    BootRecord zero{1, 0, 0};
    bool ok = prefs.putBytes("boots", &zero, sizeof(zero)) == sizeof(zero);
    prefs.end();
    return ok;
}

void MetricsService::start() {
    exporter.stop();
    activePort = config.port;
    if (activePort != 80) exporter.begin(activePort);
    pending = false;
    Serial.printf("[METRICS] Porta %u; painel web na porta 80.\n", activePort);
}
void MetricsService::process() {
    if (pending && millis() - savedAt >= 1000UL) start();
    sampleCpu();
    if (activePort != 80) exporter.handleClient();
}
bool MetricsService::save(const String& host, uint16_t port, uint16_t interval) {
    Settings next{};
    next.version = 1; next.port = port; next.interval = interval;
    host.toCharArray(next.host, sizeof(next.host));
    Preferences prefs;
    if (!prefs.begin("telemetry", false)) return false;
    bool ok = prefs.putBytes("settings", &next, sizeof(next)) == sizeof(next);
    prefs.end();
    if (ok) config = next;
    return ok;
}
void MetricsService::handleSave() {
    String host = panel->arg("host"); host.trim();
    uint16_t port, interval;
    if (!validHost(host) || !parseNumber(panel->arg("port"), 1, 65535, port) ||
        !parseNumber(panel->arg("interval"), 5, 3600, interval)) {
        show(400, "Use IPv4/DNS sem protocolo, porta 1..65535 e intervalo 5..3600 segundos.");
        return;
    }
    if (!save(host, port, interval)) {
        Serial.println("[METRICS] Falha ao salvar preferencias.");
        show(503, "Falha na NVS. Configuracao ativa preservada.");
        return;
    }
    pending = port != activePort; savedAt = millis();
    show(200, "Preferencias salvas. Painel na porta 80. Aplique o YAML no Prometheus.");
}
String MetricsService::yaml() const {
    String ip = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    return "scrape_configs:\n  - job_name: 'esp32_central'\n    scrape_interval: " + String(config.interval) +
           "s\n    metrics_path: /metrics\n    static_configs:\n      - targets: ['" +
           ip + ":" + String(config.port) + "']\n";
}
void MetricsService::show(int status, const String& notice) {
    String html = F("<!doctype html><html lang='pt-BR'><head><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'><title>Central | Prometheus</title>"
        "<style>body{font:16px system-ui;background:#10151c;color:#edf3f8;margin:24px auto;padding:16px;max-width:850px}"
        "section{background:#19212b;border:1px solid #303c49;border-radius:12px;padding:20px;margin:16px 0}"
        "a{color:#65e0be}label{display:block;margin:16px 0 6px}input{box-sizing:border-box;width:100%;padding:12px;font:inherit}"
        "button{padding:12px;margin-top:16px;font:inherit}pre{white-space:pre-wrap;overflow-wrap:anywhere}</style>"
        "</head><body><h1>Prometheus</h1><a href='/'>Voltar ao painel</a>");
    if (notice.length()) html += "<section role='status'>" + escapeHtml(notice) + "</section>";
    html += F("<section><h2>Preferencias persistentes</h2><form method='post' action='/prometheus'>"
        "<label for='host'>Endereco do servidor Prometheus (IPv4 ou DNS)</label>"
        "<input id='host' name='host' maxlength='253' required value='");
    html += escapeHtml(String(config.host));
    html += "'><label for='port'>Porta de metricas do ESP32</label>"
            "<input id='port' name='port' type='number' min='1' max='65535' required value='" + String(config.port);
    html += "'><label for='interval'>Intervalo sugerido de coleta (segundos)</label>"
            "<input id='interval' name='interval' type='number' min='5' max='3600' required value='" + String(config.interval);
    html += F("'><button>Salvar nas preferencias</button></form>"
        "<p>Coleta HTTP: o servidor consulta este ESP32. O endereco salvo e uma referencia, "
        "nao configura envio ativo nem restringe clientes. Nao informe protocolo ou caminho.</p>"
        "<p>O painel permanece na porta 80. A porta de metricas muda cerca de um segundo depois de salvar, "
        "sem reboot. Se escolher 80, /metrics compartilha o painel.</p></section>"
        "<section><h2>Configuracao do servidor</h2><p>Adicione o job no servidor e recarregue a configuracao. "
        "Salvar aqui nao configura o Prometheus automaticamente.</p><pre>");
    html += escapeHtml(yaml());
    html += F("</pre><a href='/prometheus.yml'>Baixar YAML</a></section>"
        "<section><h2>Fontes reais</h2><p>Sistema, Wi-Fi, CPU, memoria, GPIO do LED e SD. "
        "Temperatura interna nao e temperatura ambiente. Idade do clima so aparece apos dados validos.</p>"
        "<p>Sem integracao de energia, bateria, sensores ambientais locais, reles, ADC ou MQTT: essas series "
        "sao omitidas, sem zeros simulados. O SD usa estatisticas em cache; nao e lido por coleta.</p>"
        "<p>Boot/watchdog persistem; HTTP, desconexoes e contadores do SD reiniciam no boot. "
        "Use up do Prometheus para detectar o ESP desligado. Endpoints sem autenticacao: use rede confiavel.</p></section>");
    if (!countersOk) html += F("<section role='alert'>Contadores persistentes indisponiveis. Consulte a Serial.</section>");
    html += "</body></html>";
    panel->sendHeader("Cache-Control", "no-store");
    panel->send(status, "text/html; charset=utf-8", html);
}

void MetricsService::sampleCpu() {
#if configUSE_TRACE_FACILITY && configGENERATE_RUN_TIME_STATS
    if (millis() - sampledAt < 5000UL) return;
    sampledAt = millis();
    if (uxTaskGetNumberOfTasks() > 64) {
        cpuUsage = NAN; baseline = false;
        Serial.println("[METRICS] CPU indisponivel: mais de 64 tarefas.");
        return;
    }
    configRUN_TIME_COUNTER_TYPE total = 0;
    UBaseType_t count = uxTaskGetSystemState(tasks, 64, &total);
    int64_t now = esp_timer_get_time();
    uint64_t idleDelta = 0;
    bool foundAll = count != 0;
    for (BaseType_t core = 0; core < portNUM_PROCESSORS; ++core) {
        TaskHandle_t idleTask = xTaskGetIdleTaskHandleForCore(core);
        bool found = false;
        for (UBaseType_t i = 0; i < count; ++i) if (tasks[i].xHandle == idleTask) {
            auto idle = tasks[i].ulRunTimeCounter;
            idleDelta += static_cast<configRUN_TIME_COUNTER_TYPE>(idle - previousIdle[core]);
            previousIdle[core] = idle; found = true; break;
        }
        foundAll = foundAll && found;
    }
    // Runtime ESP-IDF em microssegundos, normalizado pela capacidade de todos os nucleos.
    uint64_t capacity = (now - previousTime) * portNUM_PROCESSORS;
    cpuUsage = baseline && foundAll && capacity && idleDelta <= capacity
        ? 100.0 * (1.0 - static_cast<double>(idleDelta) / capacity) : NAN;
    previousTime = now; baseline = foundAll;
#endif
}

void MetricsService::sendMetrics(WebServer& web) {
    uint32_t started = millis();
    lastExportAt = started;
    lastFree8 = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    lastLargest8 = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    web.sendHeader("Cache-Control", "no-store");
    if (lastFree8 < 1024 || lastLargest8 < 512) {
        ++exportsFailed;
        lastExportFailed = true;
        web.send(503, "text/plain; charset=utf-8", "Memoria de 8 bits insuficiente para metricas.\n");
        lastExportDurationMs = millis() - started;
        return;
    }
    web.setContentLength(CONTENT_LENGTH_UNKNOWN);
    web.send(200, "text/plain; version=0.0.4; charset=utf-8", "");
    auto client = web.client();
    client.setNoDelay(true);
    MetricsChunkStream stream(client);
    exposition(stream);
    lastExportFailed = !stream.flushBlock();
    if (lastExportFailed) { ++exportsFailed; ++errors; }
    else ++exportsOk;
    lastExportDurationMs = millis() - started;
    // WebServer finaliza a resposta chunked com o bloco zero ao retornar.
}
void MetricsService::exposition(Print& out) const {
    auto label = [](const char* name, const String& value) {
        return String(name) + "=\"" + escapeLabel(value) + "\"";
    };
    auto sample = [&](const char* name, const String& value, const String& labels = "",
                      const char* type = "gauge", bool describe = true) {
        if (describe) {
            out.print("# HELP "); out.print(name); out.print(" Central: "); out.print(name); out.print(".\n");
            out.print("# TYPE "); out.print(name); out.print(' '); out.print(type); out.print('\n');
        }
        out.print(name); out.print("{device=\""); out.print(device); out.print('"');
        if (labels.length()) { out.print(','); out.print(labels); }
        out.print("} "); out.print(value); out.print('\n');
    };
    bool online = WiFi.status() == WL_CONNECTED;
    sample("esp_info", "1", label("chip", ESP.getChipModel()) + ",model=\"ESP32-2432S028-Central\"");
    sample("esp_firmware_version", "1", label("version", CommandHandler::obterVersaoAutomatica()) +
           "," + label("build", String(__DATE__) + " " + __TIME__));
    if (online) sample("esp_ip_address", "1", label("ip", WiFi.localIP().toString()) + ",interface=\"sta\"");
    if ((WiFi.getMode() & WIFI_AP) && WiFi.softAPIP())
        sample("esp_ip_address", "1", label("ip", WiFi.softAPIP().toString()) + ",interface=\"ap\"", "gauge", !online);
    sample("esp_mac_address", "1", label("mac", device));
    sample("esp_status_online", online ? "1" : "0");
    sample("esp_uptime_seconds", String(esp_timer_get_time() / 1000000.0, 3));
    sample("esp_last_restart_reason", "1", label("reason", String(static_cast<int>(esp_reset_reason()))));
    if (countersOk) {
        sample("esp_boot_count", String(boot.boots));
        sample("esp_reboot_count", String(boot.boots ? boot.boots - 1 : 0));
        sample("esp_watchdog_resets", String(boot.watchdogs));
    }
    if (online) {
        sample("esp_wifi_rssi_dbm", String(WiFi.RSSI()));
        sample("esp_wifi_channel", String(WiFi.channel()));
        int8_t power;
        if (esp_wifi_get_max_tx_power(&power) == ESP_OK) sample("esp_wifi_tx_power", String(power / 4.0f, 2));
    }
    sample("esp_wifi_disconnect_count", String(disconnects.load()));
    auto udpStats = udp.snapshotMetricas();
    sample("esp_udp_listener_up", udpStats.listenerUp ? "1" : "0");
    sample("esp_udp_packets_received_total", String(udpStats.pacotesRecebidos), "", "counter");
    sample("esp_udp_replies_sent_total", String(udpStats.respostasEnviadas), "", "counter");
    sample("esp_udp_send_errors_total", String(udpStats.errosEnvio), "", "counter");
    sample("esp_cpu_frequency_mhz", String(ESP.getCpuFreqMHz()));
    if (isfinite(cpuUsage)) sample("esp_cpu_usage_percent", String(cpuUsage, 3));
    float temp = temperatureRead();
    if (isfinite(temp)) sample("esp_cpu_temperature_celsius", String(temp, 2));
    auto weather = ClimaManager::snapshot();
    if (weather.sincronizado) {
        if (isfinite(weather.temperatura))
            sample("sensor_temperature_celsius", String(weather.temperatura, 2),
                   "source=\"open_meteo\",location=\"porto_alegre\"");
        sample("sensor_humidity_percent", String(weather.umidade), "location=\"porto_alegre\"");
        sample("esp_weather_data_age_seconds", String((esp_timer_get_time() - weather.ultimaAtualizacaoUs) / 1000000.0, 3));
    }
    sample("esp_task_count", String(uxTaskGetNumberOfTasks()));
    sample("esp_free_rtos_tasks", String(uxTaskGetNumberOfTasks()));
    sample("esp_heap_free_bytes", String(ESP.getFreeHeap()));
    sample("esp_heap_total_bytes", String(ESP.getHeapSize()));
    sample("esp_heap_minimum_free_bytes", String(ESP.getMinFreeHeap()));
    sample("esp_heap_8bit_free_bytes", String(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));
    sample("esp_heap_8bit_largest_free_block_bytes", String(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));
    sample("esp_metrics_exports_total", String(exportsOk), "", "counter");
    sample("esp_metrics_export_errors_total", String(exportsFailed), "", "counter");
    sample("esp_screensaver_active", central.protecaoTelaAtiva() ? "1" : "0");
    sample("esp_psram_free_bytes", String(ESP.getFreePsram()));
    sample("esp_psram_total_bytes", String(ESP.getPsramSize()));
    if (partitionBytes) sample("esp_flash_usage_percent", String(100.0 * imageBytes / partitionBytes, 3));
    sample("gpio_digital_input", digitalRead(0) == HIGH ? "1" : "0", "pin=\"0\"");
    sample("gpio_digital_output", digitalRead(LED_PINOBLE) == HIGH ? "1" : "0", label("pin", String(LED_PINOBLE)));
    sample("application_requests_total", String(requests), "", "counter");
    sample("application_errors_total", String(errors), "", "counter");
    sample("application_response_time_ms", String(responseMs, 3));
    SdMetricsSnapshot sd;
    bool available = SdManager::snapshotMetricas(sd);
    sample("sdcard_metrics_available", available ? "1" : "0");
    if (available) {
        sample("sdcard_mounted", sd.pronto ? "1" : "0");
        sample("sdcard_log_records_total", String(sd.registros), "", "counter");
        sample("sdcard_log_errors_total", String(sd.falhasLog), "", "counter");
        if (sd.pronto) {
            sample("sdcard_capacity_bytes", String(sd.capacidade));
            sample("sdcard_total_bytes", String(sd.total));
            sample("sdcard_used_bytes", String(sd.usado));
            if (sd.usado <= sd.total) sample("sdcard_free_bytes", String(sd.total - sd.usado));
            if (sd.total && sd.usado <= sd.total)
                sample("sdcard_usage_percent", String(100.0 * sd.usado / sd.total, 3));
            sample("sdcard_spi_frequency_hz", String(sd.frequencia));
            sample("sdcard_log_size_bytes", String(sd.tamanhoLog));
        }
    }
}
