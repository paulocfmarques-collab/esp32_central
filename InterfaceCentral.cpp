#include "InterfaceCentral.h"
#include "UdpComm.h"

extern UdpComm udp;

extern "C" {
  SemaphoreHandle_t mutexSPI = NULL;
}
InterfaceCentral::InterfaceCentral(Display& displayRef)
  : _renderer(displayRef), _touch(), _modoOperacao(MODO_INICIAL), 
    _escravoAtivoAlvo(0), _paginaAtual(0), _modoRespostaAtivo(false), 
    _yTerminalDinamic(60), _ultimoRelogioTS(0), _ultimoWifiTS(0), 
    _tempoAberturaRespostaTS(0), _statusAtual("Selecione o destino de operacao..."), otaGravando(false),
    _ultimoToqueAtividadeTS(millis()), _protecaoTelaAtiva(false), _scrollOffsetLinhas(0), _mensagemRespostaCompleta("") {}

void InterfaceCentral::inicializar() {
    if (mutexSPI == NULL) {
        mutexSPI = xSemaphoreCreateMutex();
    }
    _touch.inicializar();
}
int InterfaceCentral::obterTotalPaginasDoModo() const {
    return (_modoOperacao == MODO_INICIAL) ? 0 : 5;
}

void InterfaceCentral::renderizarTela() {
    _modoRespostaAtivo = false;
    
    // Solicita o controle do barramento de pixels de forma segura
    if (mutexSPI != NULL && xSemaphoreTake(mutexSPI, pdMS_TO_TICKS(10)) == pdTRUE) {
        _renderer.limparPainelConteudo();
        _renderer.desenharCabecalhoFixo();
        
        _ultimoRelogioTS = 0; _ultimoWifiTS = 0;
        _renderer.atualizarRelogio(_ultimoRelogioTS);
        _renderer.atualizarSinalWifi(_ultimoWifiTS);

        if (_modoOperacao == MODO_INICIAL) {
            _renderer.desenharMenuIps(udp.obterIpEscravo(1), udp.obterIpEscravo(2));
        } else {
            _renderer.desenharPainelPaginas((int)_modoOperacao, _escravoAtivoAlvo, _paginaAtual, udp.obterIpEscravo(1), udp.obterIpEscravo(2), _statusAtual);
        }
        xSemaphoreGive(mutexSPI); // Devolve o controle assim que terminar o desenho
    }
}

const char* InterfaceCentral::escanearToque() {
    int x = 0, y = 0;
    if (!_touch.verificarToqueLegitimo(x, y)) return nullptr;

    if (_protecaoTelaAtiva) {
        _protecaoTelaAtiva = false;
        _ultimoToqueAtividadeTS = millis(); 
        digitalWrite(21, HIGH); 
        _modoOperacao = MODO_INICIAL; _paginaAtual = 0; _escravoAtivoAlvo = 0; _modoRespostaAtivo = false;
        _statusAtual = "Selecione o destino de operacao...";
        renderizarTela();
        return nullptr; 
    }

    _ultimoToqueAtividadeTS = millis();

    // ─── GERENCIAMENTO DE SCROLL TÁTIL NA TELA DE RESPOSTA ───
    if (_modoRespostaAtivo) 
    {
        // Se tocou na região das setas (X entre 265 e 320)
        if (x >= 265) {
            _tempoAberturaRespostaTS = millis(); // Renova os 30 segundos de visualização
            
            // Verifica se clicou na Seta Superior (Y entre 45 e 100)
            if (y >= 45 && y <= 100) {
                if (_scrollOffsetLinhas > 0) _scrollOffsetLinhas--;
            }
            // Verifica se clicou na Seta Inferior (Y entre 105 e 160)
            else if (y >= 105 && y <= 160) {
                _scrollOffsetLinhas++;
            }
            
            // Re-renderiza a mensagem aplicando o pulo (Offset) de linhas desejado
            if (mutexSPI != NULL && xSemaphoreTake(mutexSPI, pdMS_TO_TICKS(10)) == pdTRUE) {
                // Monta a string pulando as primeiras N linhas
                String msgExibicao = "";
                int linhasPuladas = 0;
                int indice = 0;
                
                while (indice < _mensagemRespostaCompleta.length()) {
                    int quebra = _mensagemRespostaCompleta.indexOf('\n', indice);
                    String linha = (quebra == -1) ? _mensagemRespostaCompleta.substring(indice) : _mensagemRespostaCompleta.substring(indice, quebra);
                    indice = (quebra == -1) ? _mensagemRespostaCompleta.length() : quebra + 1;
                    
                    if (linhasPuladas >= _scrollOffsetLinhas) {
                        msgExibicao += linha + "\n";
                    } else {
                        linhasPuladas++;
                    }
                }
                
                _renderer.desenharTelaResposta(msgExibicao, _yTerminalDinamic);
                xSemaphoreGive(mutexSPI);
            }
            return nullptr; // Consome o toque e não fecha a tela
        } 
        else {
            // Se tocou fora da região das setas (no meio da tela de texto), aí sim fecha a janela verde e volta
            _modoRespostaAtivo = false;
            udp.resetarEspera(); 
            renderizarTela();
            return nullptr;
        }
    }

    // ─── CORREÇÃO CRÍTICA 1: DESTREVA O TIMEOUT AO FECHAR A TELA DE RESPOSTA ───
    if (_modoRespostaAtivo) {
        _modoRespostaAtivo = false;
        udp.resetarEspera(); 
        renderizarTela();
        return nullptr;
    }

    // ─── MAPEAMENTO INTEGRADO DA GRADE DE 4 IPS ───
    if (_modoOperacao == MODO_INICIAL) {
        if (x >= LayoutDatabase::btnLocalX && x <= (LayoutDatabase::btnLocalX + LayoutDatabase::btnLocalW) && 
            y >= LayoutDatabase::btnLocalY && y <= (LayoutDatabase::btnLocalY + LayoutDatabase::btnLocalH)) {
            _modoOperacao = MODO_LOCAL; _escravoAtivoAlvo = 0; _paginaAtual = 0; 
            _statusAtual = "Menu Local - Pagina 1"; renderizarTela(); 
            return nullptr;
        }
        if (x >= LayoutDatabase::btnIp1X && x <= (LayoutDatabase::btnIp1X + LayoutDatabase::btnIp1W) && 
            y >= LayoutDatabase::btnIp1Y && y <= (LayoutDatabase::btnIp1Y + LayoutDatabase::btnIp1H)) {
            _modoOperacao = MODO_REMOTO; _escravoAtivoAlvo = 1; _paginaAtual = 0; 
            _statusAtual = "Controle UDP -> ESP 1"; renderizarTela(); 
            return nullptr;
        }
        if (x >= LayoutDatabase::btnIp2X && x <= (LayoutDatabase::btnIp2X + LayoutDatabase::btnIp2W) && 
            y >= LayoutDatabase::btnIp2Y && y <= (LayoutDatabase::btnIp2Y + LayoutDatabase::btnIp2H)) {
            _modoOperacao = MODO_REMOTO; _escravoAtivoAlvo = 2; _paginaAtual = 0; 
            _statusAtual = "Controle UDP -> ESP 2"; renderizarTela(); 
            return nullptr;
        }
        if (x >= LayoutDatabase::btnIp3X && x <= (LayoutDatabase::btnIp3X + LayoutDatabase::btnIp3W) && 
            y >= LayoutDatabase::btnIp3Y && y <= (LayoutDatabase::btnIp3Y + LayoutDatabase::btnIp3H)) {
            _modoOperacao = MODO_REMOTO; _escravoAtivoAlvo = 3; _paginaAtual = 0; 
            _statusAtual = "Controle UDP -> ESP 3"; renderizarTela(); 
            return nullptr;
        }
        if (x >= LayoutDatabase::btnIp4X && x <= (LayoutDatabase::btnIp4X + LayoutDatabase::btnIp4W) && 
            y >= LayoutDatabase::btnIp4Y && y <= (LayoutDatabase::btnIp4Y + LayoutDatabase::btnIp4H)) {
            _modoOperacao = MODO_REMOTO; _escravoAtivoAlvo = 4; _paginaAtual = 0; 
            _statusAtual = "Controle UDP -> ESP 4"; renderizarTela(); 
            return nullptr;
        }
        return nullptr;
    }

    // Botão de Próxima Página / Voltar ao Menu Principal de IPs
    if (x >= LayoutDatabase::btnProxX && x <= (LayoutDatabase::btnProxX + LayoutDatabase::btnProxW) && 
        y >= LayoutDatabase::btnProxY && y <= (LayoutDatabase::btnProxY + LayoutDatabase::btnProxH)) {
        
        // CORREÇÃO LOGICA: Reseta o soquete de espera para os comandos remotos não prenderem a tela
        udp.resetarEspera(); 
        
        if (_paginaAtual == 4) {
            _modoOperacao = MODO_INICIAL; _paginaAtual = 0; _escravoAtivoAlvo = 0; 
            _statusAtual = "Selecione o destino de operacao...";
        } else {
            _paginaAtual++; _statusAtual = "Pagina " + String(_paginaAtual + 1);
        }
        renderizarTela(); return nullptr;
    }

    // Varre de forma dinâmica os 48 botões mapeados
    for (int i = 0; i < TOTAL_BOTOES; i++) {
        int modoBotaoNecessario = (i < 30) ? 1 : 2; 
        
        if ((int)_modoOperacao == modoBotaoNecessario && LayoutDatabase::botoesAcao[i].pagina == _paginaAtual) {
            if (x >= LayoutDatabase::botoesAcao[i].x && x <= (LayoutDatabase::botoesAcao[i].x + LayoutDatabase::botoesAcao[i].w) && 
                y >= LayoutDatabase::botoesAcao[i].y && y <= (LayoutDatabase::botoesAcao[i].y + LayoutDatabase::botoesAcao[i].h)) {
                
                if (strcmp(LayoutDatabase::botoesAcao[i].comando, "vago") == 0) return nullptr;

                udp.resetarEspera();

                _mensagemRespostaCompleta = ""; 
                _scrollOffsetLinhas = 0;


                Serial.print(F("[Touch] Comando validado e disparado: "));
                Serial.println(LayoutDatabase::botoesAcao[i].comando);
                
                return LayoutDatabase::botoesAcao[i].comando;
            }
        }
    }
    return nullptr;
}

void InterfaceCentral::exibirTelaResposta(String msg) {
    _modoRespostaAtivo = true; 
    _tempoAberturaRespostaTS = millis();
    _scrollOffsetLinhas = 0;
    
    _mensagemRespostaCompleta = msg; 
    
    if (mutexSPI != NULL && xSemaphoreTake(mutexSPI, pdMS_TO_TICKS(10)) == pdTRUE) {
        _renderer.desenharTelaResposta(msg, _yTerminalDinamic);
        xSemaphoreGive(mutexSPI);
    }
}

void InterfaceCentral::acumularMensagemResposta(String msg) {
    _tempoAberturaRespostaTS = millis();
    
    // Se a tela de resposta não estava aberta, inicia um buffer totalmente fresco
    if (!_modoRespostaAtivo) { 
        exibirTelaResposta(msg); 
        return; 
    }
    
    // Se a tela já estava aberta e este é um pacote contínuo do MESMO comando, ele acumula
    _mensagemRespostaCompleta += "\n" + msg; 
    
    if (mutexSPI != NULL && xSemaphoreTake(mutexSPI, pdMS_TO_TICKS(10)) == pdTRUE) {
        _renderer.renderizarNovaLinhaResposta(msg, _yTerminalDinamic);
        xSemaphoreGive(mutexSPI);
    }
}

void InterfaceCentral::atualizarRelogioDinamico() { 
    if (mutexSPI != NULL && xSemaphoreTake(mutexSPI, pdMS_TO_TICKS(5)) == pdTRUE) {
        _renderer.atualizarRelogio(_ultimoRelogioTS); 
        xSemaphoreGive(mutexSPI);
    }
}

void InterfaceCentral::atualizarIndicadorWifi() { 
    if (mutexSPI != NULL && xSemaphoreTake(mutexSPI, pdMS_TO_TICKS(5)) == pdTRUE) {
        _renderer.atualizarSinalWifi(_ultimoWifiTS); 
        xSemaphoreGive(mutexSPI);
    }
}

void InterfaceCentral::checarAutoFechamento() { 
    if (_modoRespostaAtivo && (millis() - _tempoAberturaRespostaTS >= AUTO_CLOSE_MS)) { 
        renderizarTela(); 
    } 
    
    unsigned long tempoInativo = millis() - _ultimoToqueAtividadeTS;

    // 🔥 PRE-AVISO AMRELO: Faltando 10 segundos (290 milissegundos inativo) avisa no rodapé
    if (!_protecaoTelaAtiva && _modoOperacao != MODO_INICIAL && tempoInativo >= 290000 && tempoInativo < TIMEOUT_PROTECAO_MS) {
        static unsigned long ultimoBlinkRodape = 0;
        static bool inverterCorAviso = false;
        
        if (millis() - ultimoBlinkRodape >= 1000) {
            ultimoBlinkRodape = millis();
            inverterCorAviso = !inverterCorAviso;
            imprimirRodape("[ TELA ENTRANDO EM MODO DE DESCANSO... ]", inverterCorAviso ? TFT_YELLOW : TFT_RED);
        }
    }
    
    // Desligamento total ao atingir 5 minutos
    if (!_protecaoTelaAtiva && !otaGravando && tempoInativo >= TIMEOUT_PROTECAO_MS) {
        _protecaoTelaAtiva = true;
        
        if (mutexSPI != NULL && xSemaphoreTake(mutexSPI, pdMS_TO_TICKS(10)) == pdTRUE) {
            _renderer.limparPainelConteudo();
            _renderer.getDriver().fillScreen(TFT_BLACK);
            xSemaphoreGive(mutexSPI);
        }
        digitalWrite(21, LOW); 
        Serial.println(F("[Sistema] Protecao de tela ativa por inatividade (5 min)."));
    }
}

void InterfaceCentral::imprimirRodape(String msg, uint16_t cor) { _renderer.desenharRodape(msg, cor); }
void InterfaceCentral::desligarDisplayFisico() { _renderer.limparPainelConteudo(); digitalWrite(21, LOW); }
