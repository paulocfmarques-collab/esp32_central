#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <esp_ota_ops.h>
#include <freertos/task.h>
#include <atomic>

class MetricsService {
public:
    struct Settings {
        uint32_t version;
        char host[254];
        uint16_t port, interval;
    };
    static bool validHost(const String& host);
    static bool parseNumber(const String& text, uint16_t minimum, uint16_t maximum, uint16_t& out);
    void begin(WebServer& web);
    void start();
    void process();
    static bool resetCounters();
private:
    Settings config{};
    struct BootRecord { uint32_t version, boots, watchdogs; } boot{};
    bool countersOk = false, pending = false;
    uint16_t activePort = 9120;
    uint32_t savedAt = 0;
    WebServer* panel = nullptr;
    WebServer exporter{9120};
    String device;
    std::atomic<uint32_t> disconnects{0};
    uint64_t requests = 0, errors = 0;
    double responseMs = 0;
    uint32_t imageBytes = 0, partitionBytes = 0;
    float cpuUsage = NAN;
#if configUSE_TRACE_FACILITY && configGENERATE_RUN_TIME_STATS
    TaskStatus_t tasks[64];
    configRUN_TIME_COUNTER_TYPE previousIdle[portNUM_PROCESSORS]{};
    int64_t previousTime = 0;
    uint32_t sampledAt = 0;
    bool baseline = false;
#endif
    void sampleCpu();
    bool save(const String& host, uint16_t port, uint16_t interval);
    void show(int status = 200, const String& notice = "");
    void handleSave();
    void sendMetrics(WebServer& web);
    String exposition() const;
    String yaml() const;
    static String escapeLabel(String value);
    static String escapeHtml(String value);
};
