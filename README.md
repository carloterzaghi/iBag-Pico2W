# iBag - Sistema de Controle de Temperatura com Pico 2 W

## 📋 Descrição do Projeto

Este projeto transforma o Raspberry Pi Pico 2 W em um **Access Point WiFi completo** que hospeda uma interface web moderna para monitoramento e controle de temperatura. A aplicação implementa um sistema inteligente de controle térmico para uma bolsa térmica (iBag) com:

- 🔥 **Controle de temperatura do aquecedor** via sensores LM35 reais
- ❄️ **Controle de temperatura do congelador** via sensores LM35 reais
- ⚡ **Controle automático de Peltier** (GPIO 15) com lógica inteligente de aquecimento/resfriamento
- 📊 **Monitoramento em tempo real** com sensores de temperatura analógicos (ADC)
- 🎯 **Detecção de movimento/vibração** via acelerômetro/giroscópio MPU6050 (I2C)
- 🧠 **Algoritmo inteligente de calibração** automática com baseline de 10 segundos
- 🌐 **Servidor DHCP integrado** - conecta automaticamente sem configuração manual
- 🚀 **Servidor HTTP customizado** com suporte completo a REST APIs
- 🔄 **Sistema de resetamento inteligente** com recalibração automática do MPU6050

## 🎯 Funcionalidades Principais

### Hardware Integrado

#### 1. Sensores de Temperatura LM35
- **GPIO 26 (ADC0)**: Sensor de temperatura do congelador
- **GPIO 27 (ADC1)**: Sensor de temperatura do aquecedor
- **Resolução**: 12-bit ADC (0-4095)
- **Precisão**: 10mV/°C (0.1°C de resolução teórica)
- **Range**: -55°C a +150°C
- **Leitura**: A cada requisição HTTP `/api/status`

#### 2. Acelerômetro/Giroscópio MPU6050
- **I2C0**: SDA=GPIO20, SCL=GPIO21
- **Frequência I2C**: 400kHz (modo rápido)
- **Endereço**: 0x68
- **Sensores**: 3 eixos acelerômetro + 3 eixos giroscópio
- **Função**: Detecção inteligente de virada brusca da comida
- **Calibração**: 10 segundos de baseline na inicialização
- **Algoritmo**: Taxa de variação do Gyro Z para distinguir rotação gradual vs. brusca

**Thresholds de Detecção:**
- `ACCEL_THRESHOLD`: 20000 (valores brutos, ~1.2g)
- `GYRO_Z_RATE_THRESHOLD`: 8000 (taxa de mudança por leitura)
- `GYRO_Z_ABSOLUTE_THRESHOLD`: 12000 (valor absoluto vs. baseline)

**Lógica de Detecção:**
1. **Rotação Rápida**: Se `abs(gyro.z - last_gyro_z) > 8000` → VIRADA DETECTADA
2. **Rotação Extrema**: Se `abs(gyro.z - baseline_gyro.z) > 12000` → VIRADA DETECTADA
3. **Aceleração Alta**: Se diferença acelerômetro > 20000 → VIRADA DETECTADA

#### 3. Relé Peltier (GPIO 15)
- **Função**: Controle ON/OFF de células Peltier para aquecimento/resfriamento
- **Lógica**: Sistema inteligente com prioridade e proteções
- **Ciclo de Espera**: 15 segundos entre mudanças de estado para estabilização
- **Proteções**: Margem de ±2°C para evitar overshooting

**Algoritmo de Controle:**
```
1. SE temp_quente < alvo_quente:
   SE temp_fria > (alvo_frio - 2°C):
      LIGAR Peltier (aquecer)
   SENÃO:
      DESLIGAR (proteção: frio demais)

2. SENÃO SE temp_fria > alvo_fria:
   SE temp_quente < (alvo_quente + 2°C):
      LIGAR Peltier (esfriar)
   SENÃO:
      DESLIGAR (proteção: quente demais)

3. SENÃO:
   DESLIGAR (ambas OK)

4. AGUARDAR 15 segundos após qualquer mudança
```

### Software Integrado

#### 1. Servidor DHCP Completo
- **Porta**: UDP 67
- **Pool de IPs**: 192.168.4.2 a 192.168.4.254
- **Protocolo 4-Way Handshake**:
  1. Cliente → DHCP DISCOVER
  2. Servidor → DHCP OFFER (message type 2)
  3. Cliente → DHCP REQUEST
  4. Servidor → DHCP ACK (message type 5)
- **Opções DHCP Fornecidas**:
  - Subnet Mask: 255.255.255.0
  - Gateway: 192.168.4.1
  - DNS Server: 192.168.4.1
  - Lease Time: Configurável
- **Suporte a Fragmentação**: Usa `pbuf_copy_partial()` para lidar com pbufs encadeados

#### 2. Servidor HTTP Customizado
- **Porta**: TCP 8000
- **API**: lwIP TCP Raw API (sem RTOS, modo NO_SYS=1)
- **Buffer de Envio**: 16 × TCP_MSS (1460 bytes)
- **Suporte a Chunked Transfer**: Para respostas grandes (ex: HTML)
- **Gerenciamento de Estado**: Struct `http_state` para transfers multi-pacote
- **Content-Type Headers**: JSON, HTML, Text

**Endpoints REST API:**

| Método | Endpoint       | Descrição                              | Resposta              |
|--------|----------------|----------------------------------------|-----------------------|
| GET    | `/`            | Interface web HTML completa            | HTML (chunked)        |
| GET    | `/api/status`  | Status atual do sistema (JSON)         | JSON (single packet)  |
| GET    | `/api/config`  | Configuração de temperaturas alvo      | JSON                  |
| POST   | `/api/config`  | Atualizar temperaturas alvo            | JSON com novo config  |
| POST   | `/api/reset`   | Resetar estado de virada + calibrar    | JSON após 10s         |

#### 3. Interface Web Moderna
- **Design**: Gradiente roxo/azul, responsivo, CSS moderno
- **JavaScript**: Fetch API com async/await
- **Auto-refresh**: Atualiza status a cada 5 segundos (quando visível)
- **Sistema de Popups**: Notificações visuais para todas as ações
- **Calibração Inteligente**: 
  - Desabilita auto-refresh durante calibração
  - Timeout de 15 segundos para operação de reset
  - Popup com contagem regressiva visual

## 🔧 Pinout e Conexões

| GPIO | Função                    | Tipo      | Detalhes                                |
|------|---------------------------|-----------|-----------------------------------------|
| 26   | ADC0                      | Input     | Sensor LM35 - Temperatura Congelador   |
| 27   | ADC1                      | Input     | Sensor LM35 - Temperatura Aquecedor    |
| 20   | I2C0 SDA                  | I/O       | MPU6050 - Dados I2C                    |
| 21   | I2C0 SCL                  | Output    | MPU6050 - Clock I2C (400kHz)           |
| 15   | Digital Output            | Output    | Relé Peltier (ON/OFF)                  |
| -    | CYW43439 WiFi (integrado) | -         | Access Point (SSID: iBag-Pico2W)       |

**Alimentação:**
- Raspberry Pi Pico 2 W: 5V via USB ou VSYS
- LM35: 5V (ou 3.3V com ajuste de leitura)
- MPU6050: 3.3V (VCC, pull-ups internos de 4.7kΩ)
- Relé: 5V (com optoacoplador recomendado)

## 🌐 Configuração de Rede

### Access Point WiFi

- **SSID**: `iBag-Pico2W`
- **Senha**: `ibag12345678`
- **IP do Pico**: `192.168.4.1`
- **Subnet Mask**: `255.255.255.0`
- **DHCP Range**: `192.168.4.2` - `192.168.4.254`

### Porta do Servidor Web

- **HTTP**: Porta `8000`
- **Acesso**: `http://192.168.4.1:8000`

**Como Conectar:**
1. Ligue o Pico 2 W
2. Aguarde ~10 segundos (LED piscando = inicializando, LED fixo = AP ativo)
3. Conecte seu dispositivo ao WiFi `iBag-Pico2W` (senha: `ibag12345678`)
4. Seu dispositivo receberá IP automaticamente via DHCP
5. Abra navegador: `http://192.168.4.1:8000`

## 📡 API REST - Documentação Completa

### 1. GET `/api/status` - Status do Sistema

**Request:**
```http
GET /api/status HTTP/1.1
Host: 192.168.4.1:8000
```

**Response (200 OK):**
```json
{
  "hot_temp": 45.3,
  "cold_temp": 12.7,
  "shaken": false,
  "peltier_on": true
}
```

**Campos:**
- `hot_temp` (float): Temperatura do aquecedor em °C (sensor LM35 GPIO 27)
- `cold_temp` (float): Temperatura do congelador em °C (sensor LM35 GPIO 26)
- `shaken` (boolean): `true` se detectou virada brusca desde último reset
- `peltier_on` (boolean): `true` se relé Peltier está ligado no momento

### 2. GET `/api/config` - Obter Configuração

**Request:**
```http
GET /api/config HTTP/1.1
Host: 192.168.4.1:8000
```

**Response (200 OK):**
```json
{
  "target_hot": 50.0,
  "target_cold": 10.0
}
```

**Campos:**
- `target_hot` (float): Temperatura alvo do aquecedor em °C
- `target_cold` (float): Temperatura alvo do congelador em °C

### 3. POST `/api/config` - Atualizar Configuração

**Request:**
```http
POST /api/config HTTP/1.1
Host: 192.168.4.1:8000
Content-Type: application/json
Content-Length: 42

{"target_hot": 60.0, "target_cold": 5.0}
```

**Validações:**
- `target_hot`: entre 0.0 e 100.0 °C
- `target_cold`: entre -20.0 e 50.0 °C

**Response (200 OK):**
```json
{
  "target_hot": 60.0,
  "target_cold": 5.0
}
```

**Response Erro (400 Bad Request):**
```json
{"error": "Invalid temperature range"}
```

### 4. POST `/api/reset` - Resetar Estado e Calibrar

**Request:**
```http
POST /api/reset HTTP/1.1
Host: 192.168.4.1:8000
```

**Comportamento:**
1. Reseta flag `shaken` para `false`
2. Inicia calibração de 10 segundos do MPU6050
3. Captura baseline de acelerômetro e giroscópio
4. Aguarda 10 segundos antes de responder
5. Durante calibração: **mantenha dispositivo ESTÁVEL**

**Response (200 OK - após 10s):**
```json
{"status": "ok"}
```

**Timeout do Cliente:** Interface web aguarda até 15 segundos

## 🚀 Compilação e Flash

### Pré-requisitos

- Visual Studio Code com extensão Raspberry Pi Pico
- Pico SDK 2.2.0 ou superior instalado
- CMake 3.13+
- Ninja build system
- GCC ARM toolchain

### Compilar Projeto

1. Abrir workspace no VS Code
2. Pressionar `Ctrl+Shift+B` (Build)
3. Ou executar task: **"Compile Project"**

**Comando Manual:**
```powershell
& "$env:USERPROFILE\.pico-sdk\ninja\v1.12.1\ninja.exe" -C build
```

### Flash no Pico 2 W

**Método 1: USB (BOOTSEL)**
1. Desconecte o Pico
2. Segure botão BOOTSEL
3. Conecte USB mantendo BOOTSEL pressionado
4. Execute task: **"Run Project"**

**Método 2: Debug Probe (SWD)**
1. Conecte Debug Probe ao Pico
2. Execute task: **"Flash"**

**Arquivo gerado:** `build/iBagPico2W.uf2`

## 🧪 Sistema de Calibração MPU6050

### Por que Calibrar?

O MPU6050 possui **drift** e **offset** que variam com:
- Temperatura ambiente
- Posição do dispositivo
- Tempo desde último power-on
- Vibração mecânica residual

A calibração captura uma **baseline** da posição atual para detectar **mudanças relativas**, não absolutas.

### Processo de Calibração (10 segundos)

```
t=0s  → Inicia calibração
      → LED: Pisca rápido (100ms ON/OFF)
      → Display: "Calibrando... Não mova!"
      
t=0-10s → Loop de 10 iterações:
          - Lê acelerômetro (x, y, z)
          - Lê giroscópio (x, y, z)
          - Aguarda 1 segundo
          - Atualiza contagem visual
          
t=10s → Salva baseline final:
        - baseline_accel.x/y/z
        - baseline_gyro.x/y/z
      → LED: Pisca lento (500ms ON/OFF)
      → Display: "Calibração completa!"
      
t>10s → Sistema pronto para detectar
        → Calcula diffs relativos ao baseline
```

### Quando Calibrar?

- ✅ **Sempre na inicialização** (automático)
- ✅ **Após mover fisicamente o dispositivo**
- ✅ **Ao clicar "Resetar Estado"** (via web)
- ✅ **Se detecções falsas frequentes**

### Detecção Pós-Calibração

Após calibração, o sistema calcula:

```c
// Taxa de mudança (rotação brusca)
gyro_z_rate = abs(current_gyro.z - last_gyro.z);

// Diferença absoluta (rotação extrema)
gyro_z_absolute = abs(current_gyro.z - baseline_gyro.z);

// Diferença acelerômetro
accel_diff = sqrt(pow(accel.x - baseline.x, 2) + 
                  pow(accel.y - baseline.y, 2) + 
                  pow(accel.z - baseline.z, 2));

// Detecção
if (gyro_z_rate > 8000 ||        // Rotação rápida
    gyro_z_absolute > 12000 ||   // Rotação extrema
    accel_diff > 20000) {        // Impacto alto
    shaken = true;
}
```

## ⚙️ Configuração Avançada (lwIP)

### lwipopts.h - Otimizações de Performance

```c
// Modo sem RTOS
#define NO_SYS                    1

// Memória principal
#define MEM_SIZE                  8000      // 8KB heap

// TCP
#define TCP_MSS                   1460      // Maximum Segment Size
#define TCP_SND_BUF              (16 * TCP_MSS)  // 23.36 KB send buffer
#define TCP_WND                  (8 * TCP_MSS)   // Receive window
#define MEMP_NUM_TCP_SEG         32        // Segmentos TCP simultâneos

// DHCP
#define LWIP_DHCP                 0         // Cliente DHCP desabilitado
#define LWIP_DHCPS                1         // Servidor DHCP habilitado (customizado)

// DNS
#define LWIP_DNS                  1         // DNS básico habilitado

// Outros
#define LWIP_NETIF_HOSTNAME       1         // Hostname: "iBag-Pico2W"
#define LWIP_HTTPD                0         // HTTP nativo desabilitado (usando customizado)
```

### Ajustes de Thresholds MPU6050

No arquivo `mpu6050.c`:

```c
// Detecção de rotação rápida (valores brutos 16-bit)
#define GYRO_Z_RATE_THRESHOLD      8000

// Detecção de rotação extrema
#define GYRO_Z_ABSOLUTE_THRESHOLD  12000

// Detecção de impacto/aceleração
#define ACCEL_THRESHOLD            20000
```

**Como ajustar:**
- ⬆️ **Aumentar valores** = menos sensível (menos falsos positivos)
- ⬇️ **Diminuir valores** = mais sensível (detecta movimentos menores)

### Ajustes de Controle Peltier

No arquivo `iBagPico2W.c`:

```c
// Margem de segurança para proteção
#define TEMP_SAFETY_MARGIN         2.0     // ±2°C

// Tempo de espera entre mudanças de estado
#define PELTIER_WAIT_TIME_MS       15000   // 15 segundos
```

**Recomendações:**
- `TEMP_SAFETY_MARGIN`: Aumentar para sistemas com inércia térmica alta
- `PELTIER_WAIT_TIME_MS`: Aumentar se Peltier liga/desliga muito frequentemente

## 🔍 Troubleshooting

### Problema: WiFi não aparece

**Sintomas:**
- LED não acende após 10 segundos
- SSID "iBag-Pico2W" não aparece na lista de redes

**Soluções:**
1. Verifique alimentação: mínimo 500mA estável
2. Reconecte USB ou use fonte externa 5V
3. Aperte Reset no Pico
4. Re-flash firmware completo
5. Verifique no terminal serial: `printf("WiFi AP ativo...");`

### Problema: Não conecta ao WiFi

**Sintomas:**
- SSID aparece mas não aceita senha
- Conecta mas não recebe IP

**Soluções:**
1. Confirme senha: `ibag12345678` (case-sensitive)
2. Esqueça rede e reconecte
3. Verifique logs DHCP no terminal serial:
   ```
   DHCP: Received DISCOVER
   DHCP: Sending OFFER to <MAC>
   DHCP: Received REQUEST
   DHCP: Sending ACK to <MAC>
   ```
4. Se DHCP não funcionar: Configure IP manualmente
   - IP: `192.168.4.2` a `192.168.4.254`
   - Máscara: `255.255.255.0`
   - Gateway: `192.168.4.1`

### Problema: Página web não carrega

**Sintomas:**
- WiFi conectado mas `http://192.168.4.1:8000` não abre
- Timeout do navegador

**Soluções:**
1. **Ping test**: `ping 192.168.4.1` (deve responder)
2. **Porta correta**: Use `http://192.168.4.1:8000` (não esqueça `:8000`)
3. **Firewall**: Desabilite temporariamente firewall do cliente
4. **Cache do navegador**: Limpe cache e tente modo anônimo
5. **Terminal serial**: Verifique mensagem `HTTP server rodando na porta 8000`

### Problema: MPU6050 não detecta movimento

**Sintomas:**
- `shaken` sempre `false` mesmo girando dispositivo
- Mensagem: `MPU6050: WHO_AM_I incorreto`

**Soluções:**
1. **Verifique conexões I2C:**
   - GPIO 20 (SDA) → MPU6050 SDA
   - GPIO 21 (SCL) → MPU6050 SCL
   - GND comum
   - MPU6050 VCC em 3.3V (não 5V!)
2. **Pull-ups:** MPU6050 tem pull-ups internos de 4.7kΩ (geralmente suficiente)
3. **Endereço I2C:** Padrão é `0x68` (AD0=GND). Se AD0=VCC, mude para `0x69` no código
4. **Teste I2C scan**: Adicione código para varrer endereços:
   ```c
   for (uint8_t addr = 0x01; addr < 0x7F; addr++) {
       int ret = i2c_write_blocking(i2c0, addr, NULL, 0, false);
       if (ret >= 0) printf("Dispositivo encontrado: 0x%02X\n", addr);
   }
   ```
5. **Recalibre:** Clique "Resetar Estado" e **não mova por 10 segundos**

### Problema: Detecções falsas (sempre `shaken=true`)

**Sintomas:**
- Flag `shaken` fica `true` mesmo com dispositivo parado
- Resetar não resolve

**Soluções:**
1. **Recalibre corretamente:**
   - Clique "Resetar Estado"
   - **NÃO TOQUE NO DISPOSITIVO** por 10 segundos completos
   - Aguarde mensagem "Calibração completa"
2. **Reduza sensibilidade** em `mpu6050.c`:
   ```c
   #define GYRO_Z_RATE_THRESHOLD      10000  // era 8000
   #define ACCEL_THRESHOLD            25000  // era 20000
   ```
3. **Verifique vibração ambiente:** 
   - Coloque dispositivo em superfície estável e isolada
   - Evite ventiladores, ar condicionado próximo
4. **Temperatura estável:** MPU6050 tem drift térmico - aguarde 5 minutos após ligar

### Problema: Peltier não liga/desliga corretamente

**Sintomas:**
- `peltier_on` sempre `false` mesmo com temperaturas fora do alvo
- Ou Peltier liga/desliga muito rápido (oscilação)

**Soluções:**
1. **Verifique leitura LM35:**
   - Terminal serial deve mostrar: `Temp Quente: XX.X°C, Temp Fria: YY.Y°C`
   - Se valores absurdos (ex: 200°C, -50°C): problema na conexão ADC
2. **Teste GPIO 15:**
   - Use LED externo em GPIO 15 para verificar ON/OFF visual
   - Verifique voltagem: HIGH = 3.3V, LOW = 0V
3. **Relé não responde:**
   - Relés 5V podem precisar optoacoplador ou transistor
   - Teste com relé 3.3V lógico
   - Adicione resistor pull-down (10kΩ) em GPIO 15
4. **Oscilação (liga/desliga rápido):**
   - Aumente `PELTIER_WAIT_TIME_MS` de 15s para 30s
   - Aumente `TEMP_SAFETY_MARGIN` de 2°C para 3°C
5. **Lógica não faz sentido:**
   - Verifique no serial as mensagens:
     ```
     Peltier: LIGADO (aquecer) - Quente=42.0 < alvo=50.0, Fria=11.0 OK
     Peltier: DESLIGADO - Ambas temperaturas OK
     ```
   - Se mensagens não aparecem: problema no loop principal

### Problema: Temperaturas LM35 incorretas

**Sintomas:**
- Temperaturas muito altas/baixas (ex: 80°C em ambiente 25°C)
- Valores negativos impossíveis
- Valores não mudam

**Soluções:**
1. **Verifique alimentação LM35:**
   - Pode ser 5V ou 3.3V
   - Se 5V: saída vai até 1.5V (150°C) - OK para ADC do Pico
2. **Conexões ADC:**
   - GPIO 26 → LM35 Vout (congelador)
   - GPIO 27 → LM35 Vout (aquecedor)
   - GND comum entre Pico e LM35
3. **Cálculo de temperatura:**
   ```c
   // Código atual em iBagPico2W.c
   float voltage = (adc_value / 4095.0f) * 3.3f;
   float temperature = voltage * 100.0f; // 10mV/°C = 0.01V/°C
   ```
   - Se LM35 alimentado em 5V mas lê via ADC 3.3V: resultado será menor que real
   - **Correção:** Adicione resistor divisor de tensão ou use 3.3V no LM35
4. **Teste manual:**
   ```c
   uint16_t raw = adc_read();
   printf("ADC raw: %d, voltage: %.3fV\n", raw, (raw/4095.0)*3.3);
   ```
   - Toque no LM35: temperatura deve subir gradualmente

### Problema: Compilação falha

**Erros comuns:**

**1. `fatal error: hardware/adc.h: No such file`**
```powershell
# Solução: Verifique Pico SDK instalado corretamente
echo $env:PICO_SDK_PATH
# Deve apontar para diretório do SDK
```

**2. `undefined reference to i2c_init`**
```cmake
# Solução: Adicione em CMakeLists.txt
target_link_libraries(iBagPico2W
    hardware_i2c
    hardware_adc
    hardware_gpio
)
```

**3. `abs() was not declared`**
```c
// Solução: Adicione no início do arquivo
#include <stdlib.h>
```

## 📚 Estrutura do Código

```
iBag-Pico2W/
├── iBagPico2W.c              # Aplicação principal
│   ├── main()                # Loop principal
│   ├── init_lm35()           # Inicializa ADC para LM35
│   ├── read_lm35()           # Lê temperatura dos sensores
│   ├── init_peltier_relay()  # Configura GPIO 15
│   ├── control_peltier()     # Lógica de controle inteligente
│   └── Callbacks HTTP        # Processa requisições
│
├── mpu6050.c / mpu6050.h     # Driver MPU6050 completo
│   ├── mpu6050_init()        # Inicializa I2C e sensor
│   ├── mpu6050_read_accel()  # Lê acelerômetro
│   ├── mpu6050_read_gyro()   # Lê giroscópio
│   ├── mpu6050_detect_shake() # Algoritmo de detecção
│   ├── mpu6050_reset_shake_detection() # Inicia calibração
│   └── mpu6050_update_calibration() # Processa calibração (10s)
│
├── simple_http_server.c/h    # Servidor HTTP customizado
│   ├── http_server_init()    # Inicializa TCP port 8000
│   ├── http_recv()           # Callback de recebimento
│   ├── http_parse_request()  # Parse GET/POST
│   └── http_send_response()  # Envia JSON/HTML
│
├── dhcp_server.c/h           # Servidor DHCP completo
│   ├── dhcp_server_init()    # Inicializa UDP port 67
│   ├── dhcp_recv()           # Processa DISCOVER/REQUEST
│   ├── dhcp_send_offer()     # Envia OFFER
│   └── dhcp_send_ack()       # Envia ACK
│
├── web_content.h             # Interface web embarcada
│   └── HTML/CSS/JS completo  # String literal 10KB+
│
├── lwipopts.h                # Configuração lwIP
├── CMakeLists.txt            # Build configuration
└── pico_sdk_import.cmake     # Import do SDK
```

## 🧠 Fluxo de Execução

### 1. Inicialização (main)

```
1. stdio_init_all()              → Serial USB
2. init_lm35()                   → ADC GPIO 26, 27
3. init_peltier_relay()          → GPIO 15 como output
4. mpu6050_init()                → I2C0 400kHz, wakeup sensor
5. Calibração automática (10s)   → Baseline MPU6050
6. cyw43_arch_init()             → WiFi driver
7. cyw43_arch_enable_ap_mode()   → AP "iBag-Pico2W"
8. dhcp_server_init()            → DHCP UDP:67
9. http_server_init()            → HTTP TCP:8000
10. Loop infinito                 → Processa lwIP + controle Peltier
```

### 2. Loop Principal (1000ms)

```
CADA 1 SEGUNDO:
1. cyw43_arch_poll()             → Processa WiFi/TCP/UDP
2. read_lm35(FREEZER)            → Lê ADC GPIO 26
3. read_lm35(HEATER)             → Lê ADC GPIO 27
4. control_peltier()             → Algoritmo de controle:
   ├── SE em espera (15s não passou) → SKIP
   ├── SENÃO → Avalia temperaturas:
   │   ├── Prioridade 1: Aquecer
   │   ├── Prioridade 2: Esfriar
   │   └── Padrão: Desligar
   └── SE estado mudou → Aguardar 15s
5. SE calibração ativa:
   └── mpu6050_update_calibration() → Processa 1 step de 10
6. SENÃO:
   └── mpu6050_detect_shake()      → Verifica movimento
7. sleep_ms(1000)
```

### 3. Requisição HTTP (exemplo /api/status)

```
CLIENTE → GET /api/status
         ↓
http_recv() callback
         ↓
Identifica método GET + path "/api/status"
         ↓
Lê temperaturas atuais (LM35)
         ↓
Lê estado shaken (MPU6050)
         ↓
Lê estado peltier_on (GPIO 15)
         ↓
Formata JSON:
{
  "hot_temp": 47.2,
  "cold_temp": 9.8,
  "shaken": false,
  "peltier_on": true
}
         ↓
tcp_write() + tcp_output()
         ↓
CLIENTE ← HTTP/1.1 200 OK
```

### 4. Detecção de Shake (MPU6050)

```
LOOP CONTÍNUO (1 segundo):
1. Lê gyro.x, gyro.y, gyro.z
2. Lê accel.x, accel.y, accel.z
3. Calcula gyro_z_rate = |gyro.z - last_gyro.z|
4. Calcula gyro_z_absolute = |gyro.z - baseline_gyro.z|
5. Calcula accel_diff = sqrt(∑(accel - baseline)²)
6. VERIFICA:
   SE gyro_z_rate > 8000:        → SHAKE = TRUE (rotação rápida)
   OU gyro_z_absolute > 12000:   → SHAKE = TRUE (rotação extrema)
   OU accel_diff > 20000:        → SHAKE = TRUE (impacto alto)
7. Salva last_gyro para próxima iteração
8. Retorna shake detectado
```

## 📊 Logs e Debugging

### Terminal Serial (115200 baud)

Conecte serial USB para ver logs em tempo real:

```
Inicializando...
ADC inicializado
MPU6050: Inicializando I2C0...
MPU6050: WHO_AM_I = 0x68 ✓
MPU6050: Dispositivo acordado
Iniciando calibração (10 segundos)...
Calibração: 1/10
Calibração: 2/10
...
Calibração: 10/10
Calibração completa! Baseline salvo.
WiFi AP ativo: iBag-Pico2W
IP: 192.168.4.1
DHCP server rodando
HTTP server rodando na porta 8000

[Loop a cada 1s]
Temp Quente: 45.2°C, Temp Fria: 12.3°C, Peltier: ON
Gyro Z: 1250 (rate: 180), Accel diff: 890, Shake: NÃO

DHCP: Received DISCOVER from aa:bb:cc:dd:ee:ff
DHCP: Sending OFFER 192.168.4.2
DHCP: Received REQUEST from aa:bb:cc:dd:ee:ff
DHCP: Sending ACK 192.168.4.2

HTTP: GET /api/status
HTTP: Enviando JSON status
HTTP: GET /
HTTP: Enviando HTML (chunked, 10245 bytes)

Peltier: LIGADO (aquecer) - Quente=42.0 < alvo=50.0, Fria=11.0 OK
[15s depois]
Peltier: DESLIGADO - Quente=50.5 atingiu alvo
```

### Debug Visual (LED Onboard)

O LED integrado do Pico pisca para indicar estado:

- **Piscando rápido (100ms)**: Calibrando MPU6050 (10s)
- **Piscando lento (500ms)**: WiFi AP ativo, aguardando conexões
- **Fixo**: Cliente conectado e interface web aberta

## 🎨 Personalização da Interface Web

### Modificar Cores (web_content.h)

```css
/* Gradiente de fundo */
background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);

/* Botões */
.button {
    background: linear-gradient(135deg, #667eea, #764ba2);
}
.button:hover {
    background: linear-gradient(135deg, #764ba2, #667eea);
}
```

### Mudar Intervalo de Auto-refresh

```javascript
// Atualmente: 5000ms (5 segundos)
setInterval(checkStatus, 5000);

// Exemplo: 10 segundos
setInterval(checkStatus, 10000);
```

## 📜 Licença

Consulte o arquivo `LICENSE` para detalhes.

## 👤 Autor

Carlo - Sistema iBag com Raspberry Pi Pico 2 W

## 🔗 Referências

- [Pico SDK Documentation](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)
- [lwIP Documentation](https://www.nongnu.org/lwip/)
- [MPU6050 Datasheet](https://invensense.tdk.com/products/motion-tracking/6-axis/mpu-6050/)
- [LM35 Datasheet](https://www.ti.com/product/LM35)

### Interface Web Embutida
- Interface HTML/CSS/JavaScript moderna e responsiva (~11KB)
- Design gradient roxo/azul profissional com animações
- Totalmente embutida no firmware (sem arquivos externos)
- **Comunicação HTTP/REST** com timeout e retry automático
- Popups informativos para todas as ações
- Auto-atualização a cada 5 segundos quando status visível

### APIs REST Disponíveis

Todas as APIs incluem `Content-Length` correto e suporte a timeout.

#### 1. `GET /` ou `GET /index.html`
- Retorna a página principal do sistema (HTML embutido)
- **Tamanho:** ~11KB
- **Content-Type:** `text/html; charset=UTF-8`

#### 2. `POST /api/config`
- Configura temperaturas alvo do aquecedor e congelador
- **Content-Type:** `application/json`
- **Body JSON:**
```json
{
  "heater": 25.5,
  "freezer": -5.0
}
```
- **Resposta (HTTP 200):**
```json
{
  "status": "ok",
  "heater": 25.5,
  "freezer": -5.0
}
```

#### 3. `GET /api/status`
- Retorna status atual do sistema com dados simulados
- **Content-Type:** `application/json`
- **Resposta (HTTP 200):**
```json
{
  "heater": 26.2,
  "freezer": -4.8,
  "shaken": false
}
```
- Temperaturas variam ±2°C (aquecedor) e ±1.5°C (congelador) do valor configurado
- Estado "shaken" tem 20% de chance de ser `true` a cada consulta
- Persiste até ser resetado manualmente

#### 4. `POST /api/reset`
- Reseta o estado de "balançado/virado"
- **Content-Type:** `application/json`
- **Resposta (HTTP 200):**
```json
{
  "status": "ok"
}
```

### Servidor DHCP Integrado

O Pico 2 W atua como servidor DHCP completo:
- **Range de IPs:** `192.168.4.2` - `192.168.4.254`
- **Subnet Mask:** `255.255.255.0`
- **Gateway:** `192.168.4.1`
- **DNS:** `192.168.4.1`
- **Lease Time:** Permanente durante a sessão
- **Protocolo:** DHCP 4-way handshake (DISCOVER → OFFER → REQUEST → ACK)

## 🚀 Como Usar

### 1. Compilar o Projeto

```bash
# No PowerShell ou Terminal do VS Code
cd "C:\Users\CarloTerzaghiTuckSch\Documents\Raspberry Pi Pico 2 W\SDK\iBag-Pico2W"

# Ou use a tarefa "Compile Project" do VS Code (Ctrl+Shift+B)
```

O arquivo compilado será gerado em: `build/iBagPico2W.uf2`

### 2. Carregar no Pico 2 W

**Método 1: BOOTSEL (Manual)**
1. Desconecte o Pico 2 W do USB
2. Segure o botão **BOOTSEL** no Pico
3. Conecte o cabo USB ao computador (mantendo BOOTSEL pressionado)
4. O Pico aparecerá como um drive USB chamado `RPI-RP2`
5. Copie o arquivo `build/iBagPico2W.uf2` para o drive
6. O Pico reiniciará automaticamente e iniciará o programa

**Método 2: VS Code Task**
- Use a tarefa "Run Project" (requer picotool configurado)

### 3. Conectar ao Access Point

1. **No seu dispositivo** (celular, tablet, notebook):
   - Abra as configurações WiFi
   - Busque e conecte à rede: **`iBag-Pico2W`**
   - Senha: **`ibag12345678`** (WPA2-AES)
   - **O IP será configurado automaticamente via DHCP!** ✅

2. **Acesse a interface web:**
   - Abra qualquer navegador
   - Digite na barra de endereços: **`http://192.168.4.1:8000`**
   - A página deve carregar em ~2-3 segundos

### 4. Usar a Interface

#### Configurar Temperaturas:
1. Digite a temperatura desejada para o aquecedor (ex: 25°C)
2. Digite a temperatura desejada para o congelador (ex: -5°C)
3. Clique em "📤 Enviar Configurações"
4. Aguarde a confirmação

#### Monitorar Status:
1. Clique em "🔍 Verificar Status da Comida"
2. Veja as temperaturas atuais (simuladas com variação)
3. Veja se a comida foi balançada
4. O sistema atualiza automaticamente a cada 5 segundos

#### Resetar Estado:
- Se a comida aparecer como "balançada", clique em "🔄 Resetar Estado"

## 🔧 Configurações do Access Point

No arquivo `iBagPico2W.c`, você pode personalizar:

```c
#define AP_SSID "iBag-Pico2W"              // Nome da rede WiFi
#define AP_PASSWORD "ibag12345678"         // Senha WPA2 (mín. 8 caracteres)
#define AP_CHANNEL 1                       // Canal WiFi (1-11)
```

### Configurações de Rede (Fixas):
- **IP do Pico:** `192.168.4.1`
- **Subnet Mask:** `255.255.255.0`
- **Gateway:** `192.168.4.1`
- **Porta HTTP:** `8000`
- **Porta DHCP:** `67` (UDP)

### Configurações lwIP (`lwipopts.h`):
- **MEM_SIZE:** 8000 bytes
- **TCP_MSS:** 1460 bytes
- **TCP_SND_BUF:** 16 × MSS (~23KB)
- **TCP_WND:** 16 × MSS
- **MEMP_NUM_TCP_SEG:** 64
- **PBUF_POOL_SIZE:** 32

## 📊 Simulação de Dados

### Temperaturas
- Valores gerados com variação aleatória de ±2°C (aquecedor) e ±1.5°C (congelador)
- Baseados nos valores configurados pelo usuário
- Atualizados a cada requisição de status

### Estado de Vibração
- 20% de chance de ser marcado como "balançado" em cada verificação
- Persiste até ser resetado manualmente
- Simula detector de movimento/acelerômetro

## 🛠️ Arquitetura Técnica

### Estrutura de Arquivos

```
iBag-Pico2W/
├── iBagPico2W.c              # Aplicação principal + HTML embutido
├── simple_http_server.c      # Servidor HTTP customizado (raw TCP API)
├── simple_http_server.h      # Header do servidor HTTP
├── dhcp_server.c             # Servidor DHCP customizado (raw UDP API)
├── dhcp_server.h             # Header do servidor DHCP
├── lwipopts.h                # Configurações da stack lwIP
├── CMakeLists.txt            # Configuração de build
└── build/
    └── iBagPico2W.uf2        # Firmware compilado
```

### Stack de Rede

**Pilha de Protocolos:**
```
┌─────────────────────────────────┐
│   Aplicação (HTML + REST APIs)  │
├─────────────────────────────────┤
│   HTTP Server (TCP Raw API)     │
│   DHCP Server (UDP Raw API)     │
├─────────────────────────────────┤
│   lwIP TCP/IP Stack             │
│   (polling mode, NO_SYS=1)      │
├─────────────────────────────────┤
│   CYW43 WiFi Driver             │
│   (cyw43_arch_lwip_poll)        │
├─────────────────────────────────┤
│   Raspberry Pi Pico 2 W         │
│   (RP2350 + CYW43439)           │
└─────────────────────────────────┘
```

### Servidor HTTP Customizado

**Por que não usar httpd do lwIP?**
- O httpd padrão espera arquivos em `fsdata.c` (sistema de arquivos virtual)
- Nossa aplicação tem HTML embutido diretamente no código
- Precisávamos de controle total sobre roteamento de APIs
- Implementação customizada com raw TCP API é mais simples e direta

**Características:**
- ✅ Roteamento baseado em URI e método HTTP
- ✅ Suporte a GET e POST
- ✅ Parsing de JSON no body (POST)
- ✅ Headers HTTP completos com `Content-Length`
- ✅ Envio otimizado: chunks para >2KB, direto para <2KB
- ✅ Gerenciamento de estado com callbacks assíncronos
- ✅ Tratamento de erro com timeouts

### Servidor DHCP Customizado

**Implementação completa do protocolo DHCP:**
1. **DISCOVER** (cliente) → **OFFER** (servidor, type 2)
2. **REQUEST** (cliente) → **ACK** (servidor, type 5)

**Características:**
- ✅ Parsing de opções DHCP (option 53 para message type)
- ✅ Pool de IPs gerenciado (192.168.4.2-254)
- ✅ Atribuição de subnet mask, gateway e DNS
- ✅ Estado persistente entre OFFER e ACK
- ✅ Tratamento de pbufs fragmentados com `pbuf_copy_partial()`

## 🛠️ Bibliotecas e Dependências

### SDK do Pico
- `pico_stdlib` - Funções básicas (GPIO, time, stdio)
- `pico_cyw43_arch_lwip_poll` - WiFi + lwIP em modo polling
- Hardware: UART + USB CDC para debug serial

### lwIP Stack
- `lwip/tcp.h` - TCP raw API
- `lwip/udp.h` - UDP raw API  
- `lwip/pbuf.h` - Packet buffers
- `lwip/netif.h` - Network interfaces
- Configurado via `lwipopts.h`

### Compilador
- `arm-none-eabi-gcc` 14.2.1
- CMake 3.13+
- Ninja 1.12.1

## 🔍 Debug e Monitoramento

### Serial Monitor

O Pico envia logs detalhados via **UART** (GPIO 0/1) e **USB CDC** a **115200 baud**.

**Output típico na inicialização:**
```
=================================
iBag - Pico 2 W Access Point
=================================

Access Point iniciado!
SSID: iBag-Pico2W
Senha: ibag12345678
IP configurado: 192.168.4.1
Netmask: 255.255.255.0
Gateway: 192.168.4.1

Interface de rede ativada

╔════════════════════════════════════════════════════╗
║  AP PRONTO PARA ACEITAR CONEXÕES WiFi!           ║
╚════════════════════════════════════════════════════╝

Inicializando servidor DHCP...
DHCP Server inicializado na porta 67

Inicializando servidor HTTP customizado...
Servidor HTTP iniciado na porta 8000

=================================
🎉 SISTEMA COMPLETO ATIVO!  🎉
=================================
📡 DHCP Server: Porta 67
🌐 HTTP Server: Porta 8000
🔗 Acesse: http://192.168.4.1:8000
=================================
```

**Logs de conexão DHCP:**
```
>>> DHCP REQUEST RECEBIDO! De: 0.0.0.0:68
XID: 0xedb2a694
MAC: 30:f6:ef:7d:5d:07
Tipo de mensagem DHCP: 1 (DISCOVER)
OFFER: Oferecendo IP 192.168.4.17
Resposta DHCP enviada!

>>> DHCP REQUEST RECEBIDO! De: 0.0.0.0:68
XID: 0xedb2a694
MAC: 30:f6:ef:7d:5d:07
Tipo de mensagem DHCP: 3 (REQUEST)
ACK: Confirmando IP 192.168.4.17
Resposta DHCP enviada!
```

**Logs de requisições HTTP:**
```
╔════════════════════════════════════╗
║  CLIENTE CONECTADO!               ║
║  Nova conexão TCP na porta 8000    ║
╚════════════════════════════════════╝

>>> REQUISIÇÃO HTTP RECEBIDA! <<<
Tamanho: 471 bytes
Request:
GET / HTTP/1.1
Host: 192.168.4.1:8000
...

URI detectada: /
Servindo página principal (11340 bytes HTML, 11447 bytes total)
Total enfileirado: 11447 de 11447 bytes
HTTP: ACK recebido - 2920 bytes confirmados (total: 2920/11447)
HTTP: ACK recebido - 2920 bytes confirmados (total: 5840/11447)
HTTP: ACK recebido - 2920 bytes confirmados (total: 8760/11447)
HTTP: ACK recebido - 2687 bytes confirmados (total: 11447/11447)
HTTP: Transferência completa! Fechando conexão.
```

**Logs de APIs:**
```
>>> REQUISIÇÃO HTTP RECEBIDA! <<<
URI detectada: /api/config
POST body: {"heater":30,"freezer":-10}
Nova temperatura aquecedor: 30.0 C
Nova temperatura congelador: -10.0 C
Config atualizada: {"status":"ok","heater":30.0,"freezer":-10.0}
Resposta pequena enviada: 114 bytes
HTTP: ACK recebido - 114 bytes confirmados (total: 114/114)
HTTP: Transferência completa! Fechando conexão.
```

### Indicador LED

O LED onboard do Pico 2 W pisca a cada 10 segundos para indicar que o sistema está ativo.

## 📱 Compatibilidade

### Navegadores Suportados:
- ✅ Chrome/Chromium
- ✅ Edge
- ✅ Firefox
- ✅ Safari
- ✅ Opera
- ✅ Brave

### Dispositivos:
- 📱 Smartphones (Android/iOS)
- 💻 Notebooks (Windows/Mac/Linux)
- 📟 Tablets
- 🖥️ Desktops

## 🎨 Interface Visual

- Design moderno com gradient animado
- Cards com sombras e bordas arredondadas
- Ícones emoji para melhor usabilidade
- Popups de confirmação
- Indicadores visuais de estado (cores)
- Responsivo para mobile e desktop

## 🔐 Segurança

- WPA2-AES encryption no Access Point
- Comunicação HTTP local (não exposta à internet)
- Sem armazenamento de dados sensíveis
- Requer senha para conectar ao AP

## 💡 Melhorias Futuras e Roadmap

### 🎯 Próximas Versões

#### v1.1 - Sensores Reais
- [ ] Integrar sensor de temperatura **DS18B20** (OneWire)
- [ ] Integrar sensor de temperatura **DHT22** (ambiente)
- [ ] Integrar acelerômetro **MPU6050** (I2C) para detecção real de movimento
- [ ] Calibração de sensores via interface web

#### v1.2 - Visualização Avançada
- [ ] Gráficos de temperatura em tempo real (Chart.js)
- [ ] Histórico de últimas 24 horas
- [ ] Dashboard com estatísticas (média, min, max)
- [ ] Export de dados em CSV

#### v1.3 - Notificações e Alarmes
- [ ] Alarme sonoro via buzzer quando temperatura fora do range
- [ ] Notificações visuais piscando LEDs RGB
- [ ] Log de eventos críticos
- [ ] Alertas quando vibração detectada

#### v1.4 - Armazenamento Persistente
- [ ] Salvar configurações em Flash (LittleFS)
- [ ] Múltiplos perfis de temperatura (Verão, Inverno, Personalizado)
- [ ] Histórico persistente entre reinicializações
- [ ] Calibração de sensores salva

#### v2.0 - IoT e Cloud
- [ ] Modo Station + AP simultâneo
- [ ] Conectar a WiFi externo e sincronizar com servidor na nuvem
- [ ] MQTT para telemetria
- [ ] OTA (Over-The-Air) updates
- [ ] Integração com Home Assistant / Google Home

### 🔬 Experimentos e Ideias

- **WebSockets** para atualização em tempo real sem polling
- **HTTPS** com certificados auto-assinados
- **mDNS** para acessar via `http://ibag.local` ao invés de IP
- **Compressão gzip** nas respostas HTTP para economizar banda
- **Progressive Web App (PWA)** para instalar no celular
- **Modo Low Power** com deep sleep entre leituras
- **Bluetooth LE** como alternativa/complemento ao WiFi
- **Display OLED** local mostrando status
- **Bateria e carregamento** para uso móvel

## 🤝 Contribuindo

Este é um projeto educacional e de demonstração. Contribuições são bem-vindas!

### Como Contribuir:
1. Fork este repositório
2. Crie uma branch para sua feature (`git checkout -b feature/MinhaFeature`)
3. Commit suas mudanças (`git commit -m 'Adiciona MinhaFeature'`)
4. Push para a branch (`git push origin feature/MinhaFeature`)
5. Abra um Pull Request

### Áreas que Precisam de Ajuda:
- 📱 Melhorias na interface mobile
- 🔒 Implementação de HTTPS
- 📊 Gráficos e visualizações
- 🧪 Testes automatizados
- 📖 Documentação e tutoriais
- 🌍 Traduções (EN, ES, etc)

## 📞 Solução de Problemas

### ❌ Não consigo encontrar a rede WiFi "iBag-Pico2W"

**Possíveis causas:**
- Pico não iniciou corretamente
- Canal WiFi congestionado
- Distância muito grande

**Soluções:**
1. Verifique se o LED do Pico está piscando (a cada 10s)
2. Conecte um monitor serial (115200 baud) e veja se há mensagem "AP PRONTO"
3. Reinicie o Pico (desconecte e reconecte USB ou pressione o botão RESET)
4. Tente mudar o canal WiFi no código (linhas 18-20 de `iBagPico2W.c`)
5. Aproxime-se do Pico

### ❌ Conectado ao WiFi mas fica em "Obtendo endereço IP..."

**Possíveis causas:**
- Servidor DHCP não está respondendo
- Firewall bloqueando porta 67/UDP

**Soluções:**
1. Verifique o Serial Monitor - deve mostrar mensagens "DHCP REQUEST RECEBIDO"
2. Se não aparecer, reinicie o Pico
3. Tente desconectar e reconectar ao WiFi
4. **Alternativa:** Configure IP manualmente:
   - IP: `192.168.4.2`
   - Máscara: `255.255.255.0`
   - Gateway: `192.168.4.1`
   - DNS: `192.168.4.1`

### ❌ Conectado mas não acessa http://192.168.4.1:8000

**Possíveis causas:**
- Porta incorreta (deve ser 8000, não 80)
- Cache do navegador
- Dados móveis interferindo
- Firewall bloqueando conexão

**Soluções:**
1. Confirme que está usando **porta 8000**: `http://192.168.4.1:8000`
2. **Desative dados móveis** no celular (4G/5G pode interferir)
3. Limpe o cache do navegador (Ctrl+Shift+Del ou Ctrl+F5)
4. Tente em **modo anônimo/privado**
5. Tente outro navegador (Chrome, Firefox, Edge)
6. Verifique se consegue pingar: `ping 192.168.4.1` (Windows/Linux/Mac)

### ❌ Página carrega mas APIs dão erro "Failed to fetch"

**Possíveis causas:**
- Servidor HTTP respondendo mas fechando conexão prematuramente
- Timeout nas requisições
- Headers HTTP incorretos

**Soluções:**
1. Verifique o **Console do navegador** (F12) para detalhes do erro
2. Recarregue a página completamente (Ctrl+F5)
3. Verifique no Serial Monitor se aparece:
   ```
   URI detectada: /api/status
   API Status: {...}
   Resposta pequena enviada: 114 bytes
   HTTP: ACK recebido - 114 bytes confirmados
   HTTP: Transferência completa!
   ```
4. Se aparecer "Resposta pequena enviada" mas der erro no navegador, pode ser problema de timeout
5. Tente aumentar o timeout no código JavaScript (atualmente 5000ms)

### ❌ Compilação falha

**Erro: "SDK not found" ou "CMake error"**
- Verifique instalação do Pico SDK 2.0+
- Confirme variável de ambiente `PICO_SDK_PATH`
- No Windows: `echo $env:PICO_SDK_PATH` (PowerShell)

**Erro: "Wrong chip" ou "RP2350 required"**
- Este código é para **Pico 2 W** (RP2350), não Pico W original (RP2040)
- Verifique se selecionou o board correto no CMakeLists.txt

**Erro: "ninja: error"**
- Delete a pasta `build` completamente
- Recrie: `cmake -B build`
- Compile: `cmake --build build`

### ❌ As temperaturas não atualizam automaticamente

**Possíveis causas:**
- JavaScript parado
- Status não foi clicado inicialmente

**Soluções:**
1. Verifique o Console do navegador (F12) - procure por erros JavaScript
2. Clique primeiro em "🔍 Verificar Status da Comida"
3. Aguarde 5 segundos - a atualização é automática apenas quando o display está visível
4. Recarregue a página

### 🐛 Depuração Avançada

**Capturar tráfego de rede:**
```bash
# No Windows (Wireshark)
# 1. Instale Wireshark
# 2. Conecte ao WiFi iBag-Pico2W
# 3. Capture na interface WiFi
# 4. Filtro: ip.addr == 192.168.4.1
```

**Testes manuais com curl:**
```bash
# Teste página principal
curl http://192.168.4.1:8000/

# Teste API status
curl http://192.168.4.1:8000/api/status

# Teste API config
curl -X POST http://192.168.4.1:8000/api/config \
  -H "Content-Type: application/json" \
  -d '{"heater":30,"freezer":-10}'

# Teste API reset
curl -X POST http://192.168.4.1:8000/api/reset
```

## 📄 Licença

Este projeto é open source e está disponível sob a licença MIT.

Sinta-se livre para usar, modificar e distribuir conforme necessário.

## 🙏 Agradecimentos

- **Raspberry Pi Foundation** - Pelo incrível Pico SDK
- **lwIP Team** - Pela excelente stack TCP/IP embarcada
- **Comunidade Pico** - Por exemplos, tutoriais e suporte

## 📚 Recursos e Referências

### Documentação Oficial:
- [Raspberry Pi Pico 2 W Datasheet](https://datasheets.raspberrypi.com/picow/pico-2-w-datasheet.pdf)
- [Pico SDK Documentation](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)
- [lwIP Documentation](https://www.nongnu.org/lwip/2_1_x/index.html)
- [CYW43439 WiFi Chip](https://www.infineon.com/cms/en/product/wireless-connectivity/airoc-wi-fi-plus-bluetooth-combos/cyw43439/)

### Tutoriais e Exemplos:
- [Pico W Examples (GitHub)](https://github.com/raspberrypi/pico-examples)
- [lwIP Raw API Guide](https://www.nongnu.org/lwip/2_1_x/raw_api.html)
- [DHCP Protocol RFC 2131](https://datatracker.ietf.org/doc/html/rfc2131)

### Ferramentas Utilizadas:
- [VS Code](https://code.visualstudio.com/) - IDE
- [Raspberry Pi Pico Extension](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico)
- [arm-none-eabi-gcc](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain)
- [CMake](https://cmake.org/)
- [Ninja Build](https://ninja-build.org/)

## 👨‍💻 Autor e Informações do Projeto

**Projeto:** iBag - Sistema de Controle de Temperatura  
**Hardware:** Raspberry Pi Pico 2 W (RP2350 + CYW43439)  
**Linguagens:** C99, HTML5, CSS3, JavaScript ES6  
**Frameworks:** Pico SDK 2.0+, lwIP 2.1.x  
**Build System:** CMake + Ninja  
**Repositório:** [carloterzaghi/iBag-Pico2W](https://github.com/carloterzaghi/iBag-Pico2W)

**Versão:** 1.0.0  
**Data de Lançamento:** Novembro 2025  
**Status:** ✅ Funcional e Estável

---

## 🎓 Objetivos Educacionais

Este projeto demonstra:

### Conceitos de Embedded Systems:
- ✅ Programação bare-metal em C
- ✅ Gestão de recursos limitados (RAM, Flash)
- ✅ Programação assíncrona com callbacks
- ✅ Polling vs Interrupts

### Conceitos de Rede:
- ✅ Implementação de servidor HTTP do zero
- ✅ Protocolo DHCP completo (4-way handshake)
- ✅ Stack TCP/IP (lwIP)
- ✅ Raw sockets vs High-level APIs
- ✅ Fragmentation e reassembly de pacotes

### Conceitos Web:
- ✅ REST APIs
- ✅ Single Page Application (SPA)
- ✅ Responsive Design
- ✅ AJAX com Fetch API
- ✅ Error handling e timeouts

### Boas Práticas:
- ✅ Separação de responsabilidades (arquivos modulares)
- ✅ Logs detalhados para debugging
- ✅ Tratamento de erros robusto
- ✅ Documentação completa
- ✅ Headers HTTP corretos (Content-Length, Content-Type)

---

**Feito com ❤️ e muita paciência debugando lwIP! 😅**

*"Transformando um microcontrolador de US$ 6 em um servidor web completo!"* 🚀
