#include "SerialCommands.h"
#include "CommandHandler.h"

void SerialCommands::processar() {
    static char linha[256];
    static size_t tamanho = 0;
    static bool descartar = false;
    // Limita o trabalho para manter o loop da interface responsivo.
    for (int lidos = 0; lidos < 128 && Serial.available() > 0; ++lidos) {
        int recebido = Serial.read();
        if (recebido < 0) break;
        char c = static_cast<char>(recebido);
        if (c == '\r' || c == '\n') {
            if (descartar) {
                tamanho = 0;
                descartar = false;
                Serial.println(F("[SERIAL ERRO] Linha invalida ou maior que 255 bytes; descartada."));
                return;
            }
            if (!tamanho) continue;
            linha[tamanho] = '\0';
            String comando(linha);
            tamanho = 0;
            comando.trim();
            if (comando.length()) CommandHandler::executar(comando, false, "serial");
            return;
        }
        if (descartar) continue;
        if (c == '\b' || c == 127) {
            if (tamanho) --tamanho;
            continue;
        }
        // NUL e excesso de tamanho nunca executam um comando truncado.
        if (c == '\0' || tamanho >= sizeof(linha) - 1) {
            descartar = true;
            continue;
        }
        linha[tamanho++] = c;
    }
}
