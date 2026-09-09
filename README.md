# ESP32 Central - Sistema de Controle Híbrido Distribuído

![License](https://img.shields.io/badge/License-MIT-blue.svg)
![Language](https://img.shields.io/badge/Language-C%2B%2B-green.svg)
![Platform](https://img.shields.io/badge/Platform-ESP32-orange.svg)

## 📋 Descrição

**ESP32 Central** é um sistema de controle híbrido baseado em ESP32 que funciona como uma central de comando para gerenciar dispositivos remotos via comunicação UDP em rede local. O dispositivo oferece uma interface gráfica intuitiva com touchscreen 2.8" e suporta diversos comandos de monitoramento e controle.

O sistema é ideal para aplicações IoT, automação residencial e monitoramento de múltiplos nós ESP32 em uma rede local.

## ✨ Características Principais

### Interface Gráfica
- **Display Touchscreen 2.8"** (TFT_eSPI) com suporte a múltiplas resoluções
- Interface responsiva com 12 botões de controle organizados em 4 linhas
- Seletor de dispositivo alvo (ESP1/ESP2) com feedback visual
- Tela de resultados para exibição de informações detalhadas

### Conectividade
- **WiFi STA**: Conexão a rede existente
- **WiFi AP**: Portal de configuração automático (modo fallback)
- **UDP Communication**: Protocolo de baixa latência para controle remoto
- **Web Server Integrado**: Interface web para configuração de WiFi

### Controle e Monitoramento
- **Controle de LED**: Acionamento direto e modo blink configurável
- **Monitoramento Térmico**: Leitura de temperatura do CPU
- **Informações do Sistema**: 
  - Dados de CPU (modelo, revisão, núcleos, frequência)
  - Memória RAM (heap livre, mínimo e máximo)
  - Flash (capacidade, velocidade, tamanho do sketch)
  - Motivo do reset
  - Uptime do sistema
  - MAC address
  - Informações de rede (IP, gateway, máscara, RSSI)

### Armazenamento
- **Preferences (NVS)**: Persistência de credenciais WiFi

## 🛠️ Requisitos

### Hardware
- **Microcontrolador**: ESP32
- **Display**: TFT 2.8" com suporte a SPI
- **Touchscreen**: XPT2046 com interface SPI
- **LED**: Conectado ao pino 4
- **Alimentação**: 5V/USB ou bateria

### Software
```cpp
// Bibliotecas Arduino necessárias:
- TFT_eSPI (para display TFT)
- XPT2046_Touchscreen (para painel tátil)
- WiFi (integrada ao ESP32)
- WiFiUdp (integrada ao ESP32)
- WebServer (integrada ao ESP32)
- Preferences (integrada ao ESP32)
```

## 📌 Pinagem

| Função | Pino | Descrição |
|--------|------|-----------|
| **Touchscreen** | | |
| CLK (Clock) | 25 | SPI Clock para XPT2046 |
| MISO (Input) | 39 | Master In, Slave Out |
| MOSI (Output) | 32 | Master Out, Slave In |
| CS (Chip Select) | 33 | Chip Select |
| **Outros** | | |
| LED | 4 | LED de indicação |
| Output | 21 | Saída auxiliar |

## 🚀 Instalação

### 1. Preparar Ambiente Arduino IDE
```bash
# Instale as bibliotecas via Arduino Library Manager:
- TFT_eSPI
- XPT2046_Touchscreen
```

### 2. Configurar TFT_eSPI
Edite o arquivo `User_Setup.h` da biblioteca TFT_eSPI:
```cpp
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_CS   15
#define TFT_DC   2
```

### 3. Compilar e Fazer Upload
```bash
# No Arduino IDE:
1. Selecione a placa: ESP32 Dev Module
2. Velocidade de upload: 921600 baud
3. Compile: Sketch → Verify
4. Upload: Sketch → Upload
```

## 📡 Configuração de Rede

### Primeiro Acesso
Se o ESP32 não conseguir conectar a nenhuma rede WiFi salva:

1. O dispositivo ativa o **modo portal de configuração**
2. Conecte-se à rede: `ESP32_CENTRAL_CONFIG`
3. Acesse: `http://192.168.4.1`
4. Insira as credenciais de sua rede WiFi
5. O dispositivo reinicia e tenta conectar

### Endereços padrão de nós remotos
```cpp
ESP1: 192.168.0.120
ESP2: 192.168.0.125
Porta UDP: 4210
```

## 💻 Protocolo de Comunicação

### Formato de Comando
Comandos UDP enviados via texto simples:

| Comando | Descrição | Resposta |
|---------|-----------|----------|
| `LED_ON` | Ligar LED | Confirmação de estado |
| `LED_OFF` | Desligar LED | Confirmação de estado |
| `LED_BLINK:500` | Ativar blink (ms) | Status do blink |
| `TEMP` | Temperatura do CPU | Valor em °C |
| `CPU` | Info do processador | Modelo, núcleos, freq |
| `RAM` | Informações de memória | Heap livre/máximo |
| `FLASH` | Info da flash | Capacidade e velocidade |
| `INIT` | Motivo do reset | Último evento de reset |
| `UPTIME` | Tempo online | Milissegundos |
| `MAC` | Endereço MAC | MAC address |
| `NET_INFO` | Dados de rede | IP, Gateway, RSSI |
| `RESET_WIFI` | Limpar config WiFi | Reinicialização |

### Exemplo de Comunicação UDP (Python)
```python
import socket

# Criar socket UDP
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Enviar comando
message = "TEMP"
sock.sendto(message.encode(), ("192.168.0.120", 4210))

# Receber resposta
data, addr = sock.recvfrom(1024)
print(f"Resposta: {data.decode()}")

sock.close()
```

## 🎮 Interface do Touchscreen

### Tela Principal
```
┌─────────────────────────────────────┐
│  CENTRAL HIBRIDA TOTAL              │
├───────────────────┬─────────────────┤
│   ESP 1 (120)     │   ESP 2 (125)   │ ← Seletor de alvo
├──────┬──────┬──────┼──────┬──────┬──────┤
│ LED  │ LED  │BLINK │ VER  │INFO  │ RAM  │
│ ON   │ OFF  │ 1s   │ TEMP │ CPU  │FREE  │
├──────┼──────┼──────┼──────┼──────┼──────┤
│FLASH │ RST  │UPTIME│ END  │ REDE │ RST  │
│ INF  │MOTIV │      │ MAC  │ INFO │ WIFI │
└──────┴──────┴──────┴──────┴──────┴──────┘
```

### Tela de Resultados
- Exibe respostas dos comandos
- Suporta até 14 linhas de texto
- Toque na tela para retornar ao menu principal

## 🔧 Estrutura do Código

### Componentes Principais
- **Inicialização**: Setup de hardware, WiFi e display
- **Loop Principal**: Gerenciamento de eventos touchscreen e UDP
- **Interface Gráfica**: Renderização de UI e feedback visual
- **Controle de Comandos**: Processamento local e remoto
- **Comunicação UDP**: Envio/recebimento de pacotes

### Variáveis de Estado
```cpp
bool ESP1Selecionado      // Qual ESP está selecionado
bool blinkAtivo            // Status do LED blink
bool telaPrincipalAtiva    // Tela atual mostrada
unsigned long ultimoTouch  // Debounce de toque
```

## 📊 Desempenho

- **Latência UDP**: < 50ms
- **Taxa de Refresh UI**: ~60 FPS
- **Consumo em espera**: ~150mA
- **Consumo em operação**: ~200mA
- **Memória RAM usada**: ~120KB (dinâmica)

## 🐛 Troubleshooting

### Display não aparece
- Verifique a pinagem SPI no código
- Confirme a biblioteca TFT_eSPI está instalada
- Teste com o exemplo básico da biblioteca

### Touchscreen não responde
- Calibre o toque ajustando os valores de mapeamento (linhas 187-188)
- Verifique a pinagem XPT2046
- Teste a pressão com valor `z > 150`

### WiFi não conecta
- Confira se o SSID e senha estão corretos
- Verifique se o ESP32 está dentro do alcance
- Use o portal de configuração (modo AP)

### Comandos UDP não funcionam
- Verifique se o firewall permite UDP na porta 4210
- Confirme os IPs dos nós remotos estão corretos
- Use ferramentas como `netcat` para testar conectividade

## 📝 Licença

Este projeto está licenciado sob a Licença MIT - veja o arquivo `LICENSE` para detalhes.

## 👤 Autor

**Paulo Marques**
- GitHub: [@paulocfmarques-collab](https://github.com/paulocfmarques-collab)

## 🤝 Contribuições

Contribuições são bem-vindas! Por favor:
1. Faça um fork do projeto
2. Crie uma branch para sua feature (`git checkout -b feature/MinhaFeature`)
3. Commit suas mudanças (`git commit -m 'Adiciona MinhaFeature'`)
4. Push para a branch (`git push origin feature/MinhaFeature`)
5. Abra um Pull Request

## 📞 Suporte

Para dúvidas, sugestões ou relatos de bugs, abra uma [Issue](https://github.com/paulocfmarques-collab/esp32_central/issues) neste repositório.

## 🔗 Referências Úteis

- [Documentação ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [TFT_eSPI GitHub](https://github.com/Bodmer/TFT_eSPI)
- [XPT2046_Touchscreen GitHub](https://github.com/PaulStoffregen/XPT2046_Touchscreen)
- [Arduino IDE](https://www.arduino.cc/en/software)

---

**Desenvolvido com ❤️ para IoT e Automação**
