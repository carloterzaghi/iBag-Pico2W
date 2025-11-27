# iBag - Sistema de Controle de Temperatura com Pico 2 W

## 📋 Descrição do Projeto

Este projeto transforma o Raspberry Pi Pico 2 W em um **Access Point WiFi completo** que hospeda uma interface web moderna para monitoramento e controle de temperatura. A aplicação implementa um sistema inteligente de controle térmico para uma bolsa térmica (iBag) com:

- 🔥 **Controle de temperatura do aquecedor** via sensor LM35 real
- 🌡️ **Controle de temperatura do conservador** via sensor LM35 real
- ⚡ **Controle automático de Peltier** (GPIO 15) com lógica inteligente para ligar/desligar
- 📊 **Monitoramento em tempo real** com sensores de temperatura analógicos (ADC)
- 🎯 **Detecção de movimento/vibração** via acelerômetro/giroscópio MPU6050 (I2C)
- 🧠 **Algoritmo de calibração automática** com baseline de 10 segundos na inicialização
- 🌐 **Servidor DHCP integrado** que conecta clientes automaticamente sem configuração manual
- 🚀 **Servidor HTTP customizado** com suporte a REST APIs, construído com a Raw API do lwIP

## 🎯 Funcionalidades Principais

### Hardware Integrado

#### 1. Sensores de Temperatura LM35
- **GPIO 26 (ADC0)**: Sensor de temperatura do conservador
- **GPIO 27 (ADC1)**: Sensor de temperatura do aquecedor
- **Resolução**: 12-bit ADC (0-4095)
- **Leitura**: A cada requisição HTTP `/api/status`

#### 2. Acelerômetro/Giroscópio MPU6050
- **I2C0**: SDA=GPIO20, SCL=GPIO21
- **Frequência I2C**: 400kHz (modo rápido)
- **Endereço**: 0x68
- **Função**: Detecção inteligente de virada brusca da comida
- **Calibração**: Baseline de 10 segundos na inicialização e via API
- **Algoritmo**: Utiliza taxa de variação do Gyro Z e aceleração total para distinguir rotação gradual vs. brusca

**Thresholds de Detecção:**
- `ACCEL_THRESHOLD`: 20000 (valores brutos)
- `GYRO_Z_RATE_THRESHOLD`: 8000 (taxa de mudança por leitura)
- `GYRO_Z_ABSOLUTE_THRESHOLD`: 12000 (valor absoluto vs. baseline)

#### 3. Relé Peltier (GPIO 15)
- **Função**: Controle ON/OFF de células Peltier para aquecimento/resfriamento
- **Lógica**: Se a temperatura (aquecedor ou conservador) está fora do alvo, o relé é ligado. Ao atingir a meta, o relé desliga por um período de espera para evitar oscilações.
- **Ciclo de Espera**: `20000` ms (20 segundos) entre religamentos para estabilização.

### Software Integrado

#### 1. Servidor DHCP Customizado (Raw UDP API)
- **Porta**: UDP 67
- **Pool de IPs**: 192.168.4.2 a 192.168.4.254
- **Protocolo**: Implementa o 4-Way Handshake (DISCOVER, OFFER, REQUEST, ACK)
- **Opções Fornecidas**: Subnet Mask, Gateway, DNS Server, Lease Time

#### 2. Servidor HTTP Customizado (Raw TCP API)
- **Porta**: TCP 8000
- **API**: lwIP TCP Raw API (NO_SYS=1)
- **Roteamento**: Parse manual de URI e método (GET/POST)
- **Gerenciamento de Estado**: Callbacks assíncronos para gerenciar conexões

#### 3. Interface Web Moderna
- **Design**: Responsivo, com gradiente e CSS moderno
- **Framework**: JavaScript puro com Fetch API (async/await)
- **Auto-refresh**: Atualiza o status a cada 5 segundos
- **Notificações**: Popups visuais para todas as ações (configuração, reset, erro)
- **Calibração Inteligente**: A interface aguarda a recalibração de 10s do MPU6050, mostrando um popup informativo.

## 🔧 Pinout e Conexões

| GPIO | Função                    | Tipo      | Detalhes                                |
|------|---------------------------|-----------|-----------------------------------------|
| 26   | ADC0                      | Input     | Sensor LM35 - Temperatura Conservador   |
| 27   | ADC1                      | Input     | Sensor LM35 - Temperatura Aquecedor     |
| 20   | I2C0 SDA                  | I/O       | MPU6050 - Dados I2C                     |
| 21   | I2C0 SCL                  | Output    | MPU6050 - Clock I2C (400kHz)            |
| 15   | Digital Output            | Output    | Relé Peltier (ON/OFF)                   |
| -    | CYW43439 WiFi (integrado) | -         | Access Point (SSID: iBag-Pico2W)        |

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

## 📡 API REST - Documentação

### 1. `GET /api/status` - Status do Sistema

Retorna o status atual dos sensores.

**Request:**
```http
GET /api/status HTTP/1.1
Host: 192.168.4.1:8000
```

**Response (200 OK):**
```json
{
  "heater": 45.3,
  "freezer": 12.7,
  "shaken": false
}
```
- `heater` (float): Temperatura do aquecedor em °C.
- `freezer` (float): Temperatura do conservador em °C.
- `shaken` (boolean): `true` se detectou virada brusca desde o último reset.

### 2. `POST /api/config` - Atualizar Configuração

Define as temperaturas alvo para o aquecedor e o conservador.

**Request:**
```http
POST /api/config HTTP/1.1
Host: 192.168.4.1:8000
Content-Type: application/json

{
  "heater": 50.0,
  "freezer": 10.0
}
```

**Response (200 OK):**
```json
{
  "status": "ok",
  "heater": 50.0,
  "freezer": 10.0
}
```

### 3. `POST /api/reset` - Resetar Estado e Calibrar

Reseta a flag `shaken` para `false` e inicia um novo ciclo de calibração de 10 segundos do MPU6050.

**Atenção:** Mantenha o dispositivo estável durante a calibração. A resposta HTTP só é enviada após os 10 segundos.

**Request:**
```http
POST /api/reset HTTP/1.1
Host: 192.168.4.1:8000
```

**Response (200 OK - após 10s):**
```json
{
  "status": "ok"
}
```

## 🚀 Como Usar

### 1. Compilar e Carregar
1.  Compile o projeto usando o VS Code (Task: `Build`) ou manualmente com `ninja`. O firmware será gerado em `build/iBagPico2W.uf2`.
2.  Coloque o Pico 2 W em modo **BOOTSEL** (segure o botão BOOTSEL e conecte o cabo USB).
3.  Arraste o arquivo `build/iBagPico2W.uf2` para o drive `RPI-RP2` que aparece no seu computador.
4.  O Pico reiniciará e começará a calibração de 10 segundos (não mova o dispositivo).

### 2. Conectar ao Access Point
1.  No seu celular ou notebook, conecte-se à rede WiFi:
    - **SSID**: `iBag-Pico2W`
    - **Senha**: `ibag12345678`
2.  Seu dispositivo receberá um endereço IP automaticamente via DHCP.

### 3. Acessar a Interface
1.  Abra seu navegador de internet.
2.  Acesse o endereço: **`http://192.168.4.1:8000`**

## 🛠️ Arquitetura do Código

```
iBag-Pico2W/
├── iBagPico2W.c              # Loop principal, inicialização e lógica de controle do relé
├── mpu6050.c / .h            # Driver do MPU6050, com calibração e detecção de shake
├── simple_http_server.c / .h # Servidor HTTP customizado (Raw TCP API) para roteamento e APIs
├── dhcp_server.c / .h        # Servidor DHCP customizado (Raw UDP API)
├── web_content.h             # String com todo o conteúdo HTML/CSS/JS da interface web
├── lwipopts.h                # Configurações da stack lwIP
├── CMakeLists.txt            # Configuração de build do projeto
└── pico_sdk_import.cmake     # Import do Pico SDK
```

---

**Feito com ❤️ e muita paciência debugando lwIP! 😅**