#include "SdManager.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include <time.h>
#include <atomic>

extern "C" SemaphoreHandle_t mutexSPI;
namespace {
    SdSnapshot estado;
    std::atomic<uint32_t> registros{0}, falhasLog{0};
    bool montado = false;
    unsigned long ultimaVerificacao = 0;
    bool inicializacaoSdConcluida = false;
    struct RegistroBoot { uint32_t instante; char texto[192]; };
    RegistroBoot boot[8] = {};
    uint8_t quantidadeBoot = 0;
    uint32_t perdasBoot = 0;
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

SdSnapshot SdManager::snapshot() {
    SdSnapshot copy = estado;
    copy.registros = registros.load();
    copy.falhasLog = falhasLog.load();
    return copy;
}
bool SdManager::snapshotMetricas(SdMetricsSnapshot& out) {
    // Copia somente o cache, sem reconfigurar SPI nem aguardar uma operacao de arquivo.
    if (mutexSPI == nullptr || xSemaphoreTake(mutexSPI, 0) != pdTRUE) return false;
    out.pronto = estado.pronto;
    out.capacidade = estado.capacidade;
    out.total = estado.total;
    out.usado = estado.usado;
    out.frequencia = estado.frequencia;
    out.tamanhoLog = estado.tamanhoLog;
    out.registros = registros.load();
    out.falhasLog = falhasLog.load();
    xSemaphoreGive(mutexSPI);
    return true;
}
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
    inicializacaoSdConcluida = true;
    ultimaVerificacao = millis();
    Serial.println(estado.pronto ? "[SD] Cartao pronto." : "[SD] " + estado.erro);
    Serial.println(F("[SD] Gravando registro de inicializacao..."));
    Serial.flush();
    for (uint8_t i = 0; i < quantidadeBoot; ++i)
        registrar("BOOT", "central", "instante=" + String(boot[i].instante) + "ms | " + boot[i].texto);
    quantidadeBoot = 0;
    if (perdasBoot) registrar("BOOT", "central", "Mensagens anteriores excedentes: " + String(perdasBoot));
    registrarInicializacao(estado.pronto ? "SD: cartao pronto" : "SD: " + estado.erro);
    Serial.println(F("[SD] Registro de inicializacao concluido."));
    Serial.flush();
}
void SdManager::registrarInicializacao(const String& texto) {
    if (inicializacaoSdConcluida) { registrar("BOOT", "central", texto); return; }
    if (quantidadeBoot >= 8) { ++perdasBoot; return; }
    auto& registro = boot[quantidadeBoot++];
    registro.instante = millis();
    texto.toCharArray(registro.texto, sizeof(registro.texto));
}
bool SdManager::salvarCrash(const String& relatorio, String& caminho, String& erro) {
    caminho = "";
    if (!estado.pronto) { erro = "SD indisponivel"; return false; }
    Acesso acesso;
    if (!acesso.pronto()) { erro = "Sem acesso ao SD"; return false; }
    struct tm hora;
    char data[32];
    if (getLocalTime(&hora, 0)) strftime(data, sizeof(data), "%Y%m%d_%H%M%S", &hora);
    else snprintf(data, sizeof(data), "SEM_NTP_%lu", (unsigned long)millis());
    for (unsigned i = 0; i < 1000; ++i) {
        caminho = "/crash_" + String(data) + (i ? "_" + String(i) : String()) + ".txt";
        if (!SD.exists(caminho)) break;
        caminho = "";
    }
    if (!caminho.length()) { erro = "Nomes de arquivo esgotados"; return false; }
    File arquivo = SD.open(caminho, FILE_WRITE);
    if (!arquivo) { erro = "Falha ao criar " + caminho; return false; }
    String cabecalho = "Capturado em: " + String(data) + " | uptime=" + String(millis()) + "ms\n";
    size_t esperado = cabecalho.length() + relatorio.length();
    size_t gravado = arquivo.print(cabecalho);
    gravado += arquivo.print(relatorio);
    arquivo.flush();
    arquivo.close();
    if (gravado != esperado) { erro = "Gravacao incompleta: " + caminho; return false; }
    // Conferir o conteudo reabrindo o arquivo antes de autorizar apagar a Flash.
    arquivo = SD.open(caminho, FILE_READ);
    bool ok = arquivo && arquivo.size() == esperado;
    size_t posicao = 0;
    uint8_t bloco[128];
    while (ok && posicao < esperado) {
        size_t lidos = arquivo.read(bloco, min(sizeof(bloco), esperado - posicao));
        if (!lidos) { ok = false; break; }
        for (size_t i = 0; i < lidos; ++i, ++posicao) {
            char original = posicao < cabecalho.length() ? cabecalho[posicao] : relatorio[posicao - cabecalho.length()];
            if (bloco[i] != uint8_t(original)) { ok = false; break; }
        }
    }
    if (arquivo) arquivo.close();
    if (!ok) { erro = "Falha ao conferir " + caminho; return false; }
    erro = "";
    return true;
}
void SdManager::processar() {
    if (millis() - ultimaVerificacao < 30000) return;
    ultimaVerificacao = millis();
    Acesso acesso;
    if (acesso.pronto()) atualizarEstatisticas();
}
void SdManager::registrar(const char* evento, const String& origem, const String& texto) {
    // Sem cartao, nao repetir tres tentativas de montagem para cada pacote.
    if (!estado.pronto) { ++falhasLog; return; }
    Acesso acesso;
    if (!acesso.pronto()) { ++falhasLog; return; }
    struct tm hora;
    char data[24] = "SEM_NTP";
    if (getLocalTime(&hora, 0)) strftime(data, sizeof(data), "%Y-%m-%d %H:%M:%S", &hora);
    String registro = String(data) + " | up=" + String(millis()) + "ms | " + evento + " | " + escapar(origem) + " | " + escapar(texto) + "\n";
    File log = SD.open("/log.txt", FILE_APPEND);
    if (!log || log.isDirectory()) {
        if (log) log.close();
        ++falhasLog; falha("Falha ao abrir /log.txt"); return;
    }
    uint32_t tamanhoAnterior = log.size();
    size_t gravado = log.print(registro);
    log.flush();
    estado.tamanhoLog = log.size();
    log.close();
    if (gravado != registro.length() || estado.tamanhoLog != tamanhoAnterior + registro.length()) {
        ++falhasLog; falha("Falha de escrita / cartao cheio"); return;
    }
    ++registros;
}
