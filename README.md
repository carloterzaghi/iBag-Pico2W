# iBag - Sistema de Controle de Temperatura com Pico 2 W

## 📋 Descrição do Projeto

Este projeto transforma o Raspberry Pi Pico 2 W em um **Access Point WiFi completo** que hospeda uma interface web moderna para monitoramento e controle de temperatura. A aplicação simula um sistema de controle térmico para uma bolsa térmica (iBag) com:

- 🔥 Controle de temperatura do aquecedor
- ❄️ Controle de temperatura do congelador  
- 📊 Monitoramento em tempo real com dados simulados
- ⚠️ Detecção de movimento/vibração (simulado)
- 🌐 **Servidor DHCP integrado** - conecta automaticamente sem configuração manual
- 🚀 **Servidor HTTP customizado** com suporte completo a REST APIs

## 🎯 Funcionalidades

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
