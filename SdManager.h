#pragma once
#include <Arduino.h>
#include <SD.h>

struct SdSnapshot {
    bool pronto = false;
    String tipo = "--";
    String erro;
    uint64_t capacidade = 0, total = 0, usado = 0;
    uint32_t frequencia = 0, registros = 0, falhasLog = 0, tamanhoLog = 0;
};

class SdManager {
public:
    static constexpr int CLK = 18, MISO = 19, MOSI = 23, CS = 5;
    // Mantem o SPI do display protegido e restaura sua pinagem em todo retorno.
    class Acesso {
    public:
        explicit Acesso(bool remontar = false);
        ~Acesso();
        bool pronto() const { return _pronto; }
        Acesso(const Acesso&) = delete;
        Acesso& operator=(const Acesso&) = delete;
    private:
        bool _travado = false, _pronto = false;
    };
    static void inicializar();
    static void processar();
    static SdSnapshot snapshot();
    static void atualizarEstatisticas(); // somente dentro de Acesso
    static void registrar(const char* evento, const String& origem, const String& texto);
    static void falha(const String& erro);
};
