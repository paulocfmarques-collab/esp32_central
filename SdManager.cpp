#include "SdManager.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include <time.h>

extern "C" SemaphoreHandle_t mutexSPI;
namespace {
    SdSnapshot estado;
    bool montado = false;
    unsigned long ultimaVerificacao = 0;
    String escapar(const String& texto) {
        String out; out.reserve(texto.length() + 16);
        for (size_t i = 0; i < texto.length(); ++i) {
            char c = texto[i];
            if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else out += c;
        }
        return out;
    }
}

SdManager::Acesso::Acesso(bool remontar) {
    if (mutexSPI == nullptr || xSemaphoreTake(mutexSPI, pdMS_TO_TICKS(200)) != pdTRUE) return;
    _travado = true;
    auto& spi = TFT_eSPI::getSPIinstance();
    pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);
    pinMode(CS, OUTPUT); digitalWrite(CS, HIGH);
    spi.end();
    if (!spi.begin(CLK, MISO, MOSI, CS)) {
        SdManager::falha("Falha ao configurar SPI do SD");
        return;
    }
    if (remontar || !estado.pronto) {
        if (montado) SD.end();
        montado = false;
        estado.pronto = false;
        const uint32_t velocidades[] = {4000000, 1000000, 400000};
        for (uint32_t hz : velocidades) {
            if (SD.begin(CS, spi, hz, "/sd", 5, false) && SD.cardType() != CARD_NONE) {
                montado = estado.pronto = true;
                estado.frequencia = hz;
                estado.erro = "";
                SdManager::atualizarEstatisticas();
                break;
            }
            SD.end();
        }
        if (!montado) {
            estado.erro = "Cartao ausente ou FAT indisponivel";
            estado.tipo = "--";
            estado.capacidade = estado.total = estado.usado = 0;
            estado.frequencia = 0;
        }
    }
    _pronto = estado.pronto;
}

SdManager::Acesso::~Acesso() {
    if (!_travado) return;
    digitalWrite(CS, HIGH);
    auto& spi = TFT_eSPI::getSPIinstance();
    spi.end();
    spi.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, -1);
    digitalWrite(TFT_CS, HIGH);
    xSemaphoreGive(mutexSPI);
}

SdSnapshot SdManager::snapshot() { return estado; }
void SdManager::falha(const String& erro) { estado.pronto = false; estado.erro = erro; }
void SdManager::atualizarEstatisticas() {
    if (!estado.pronto) return;
    switch (SD.cardType()) {
        case CARD_MMC: estado.tipo = "MMC"; break;
        case CARD_SD: estado.tipo = "SDSC"; break;
        case CARD_SDHC: estado.tipo = "SDHC"; break;
        default: falha("Cartao indisponivel"); return;
    }
    estado.capacidade = SD.cardSize();
    estado.total = SD.totalBytes();
    estado.usado = SD.usedBytes();
    File log = SD.open("/log.txt", FILE_READ);
    estado.tamanhoLog = log ? log.size() : 0;
    if (log) log.close();
}
void SdManager::inicializar() {
    { Acesso acesso; }
    ultimaVerificacao = millis();
    Serial.println(estado.pronto ? "[SD] Cartao pronto." : "[SD] " + estado.erro);
    registrar("SISTEMA", "central", "Inicializacao da Central");
}
void SdManager::processar() {
    if (millis() - ultimaVerificacao < 30000) return;
    ultimaVerificacao = millis();
    Acesso acesso;
    if (acesso.pronto()) atualizarEstatisticas();
}
void SdManager::registrar(const char* evento, const String& origem, const String& texto) {
    // Sem cartao, nao repetir tres tentativas de montagem para cada pacote.
    if (!estado.pronto) { ++estado.falhasLog; return; }
    Acesso acesso;
    if (!acesso.pronto()) { ++estado.falhasLog; return; }
    struct tm hora;
    char data[24] = "SEM_NTP";
    if (getLocalTime(&hora, 0)) strftime(data, sizeof(data), "%Y-%m-%d %H:%M:%S", &hora);
    String registro = String(data) + " | up=" + String(millis()) + "ms | " + evento + " | " + escapar(origem) + " | " + escapar(texto) + "\n";
    File log = SD.open("/log.txt", FILE_APPEND);
    if (!log || log.isDirectory()) {
        if (log) log.close();
        ++estado.falhasLog; falha("Falha ao abrir /log.txt"); return;
    }
    uint32_t tamanhoAnterior = log.size();
    size_t gravado = log.print(registro);
    log.flush();
    estado.tamanhoLog = log.size();
    log.close();
    if (gravado != registro.length() || estado.tamanhoLog != tamanhoAnterior + registro.length()) {
        ++estado.falhasLog; falha("Falha de escrita / cartao cheio"); return;
    }
    ++estado.registros;
}
