#include "CrashDiagnostics.h"
#include "CommandHandler.h"
#include "SdManager.h"
#include <esp_core_dump.h>

String CrashDiagnostics::lerArquivarEApagar()
{
    String resultado = "Ultimo reset: " + CommandHandler::obterMotivoReset() + "\n";
#if CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH && CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF
    esp_err_t erro = esp_core_dump_image_check();
    if (erro != ESP_OK)
        return resultado + "Sem core dump valido: " + esp_err_to_name(erro) + "\n";
    char motivo[200] = {};
    if (esp_core_dump_get_panic_reason(motivo, sizeof(motivo)) == ESP_OK)
        resultado += "Panic salvo: " + String(motivo) + "\n";
    esp_core_dump_summary_t resumo = {};
    erro = esp_core_dump_get_summary(&resumo);
    if (erro != ESP_OK)
        return resultado + "Falha ao ler resumo: " + esp_err_to_name(erro) + "\n";
    char endereco[32];
    snprintf(endereco, sizeof(endereco), "0x%08lx", (unsigned long)resumo.exc_pc);
    resultado += "Tarefa: " + String(resumo.exc_task) + "\nPC: " + endereco + "\nBacktrace:";
    for (uint32_t i = 0; i < resumo.exc_bt_info.depth &&
                         i < sizeof(resumo.exc_bt_info.bt) / sizeof(resumo.exc_bt_info.bt[0]); ++i) {
        snprintf(endereco, sizeof(endereco), " 0x%08lx", (unsigned long)resumo.exc_bt_info.bt[i]);
        resultado += endereco;
    }
    resultado += "\nBacktrace corrompido: " + String(resumo.exc_bt_info.corrupted ? "SIM" : "NAO");
    resultado += "\nELF SHA256: " + String(reinterpret_cast<const char*>(resumo.app_elf_sha256)) + "\n";
    String caminho, erroSd;
    if (!SdManager::salvarCrash(resultado, caminho, erroSd))
        return resultado + "Falha ao arquivar crash: " + erroSd + "\nRegistro interno preservado.\n";
    resultado += "Crash salvo no SD: " + caminho + "\n";
    erro = esp_core_dump_image_erase();
    resultado += erro == ESP_OK ? "Registro de crash apagado apos leitura.\n"
                               : "Falha ao apagar registro de crash: " + String(esp_err_to_name(erro)) + "\n";
#else
    resultado += "Core dump nao habilitado neste firmware.\n";
#endif
    return resultado;
}
