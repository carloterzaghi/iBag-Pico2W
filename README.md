# iBag - Sistema de Controle de Temperatura com Pico 2 W

## 📋 Descrição do Projeto

Este projeto transforma o Raspberry Pi Pico 2 W em um **Access Point WiFi** que hospeda uma interface web completa para monitoramento e controle de temperatura. A aplicação simula um sistema de controle térmico para uma bolsa térmica (iBag) com:

- 🔥 Controle de temperatura do aquecedor
- ❄️ Controle de temperatura do congelador  
- 📊 Monitoramento em tempo real com dados simulados
- ⚠️ Detecção de movimento/vibração (simulado)

## 🎯 Funcionalidades

### Interface Web Embutida
- Interface HTML/CSS/JavaScript moderna e responsiva
- Design gradient roxo/azul profissional
- Totalmente funcional sem necessidade de arquivos externos
- **Comunicação direta HTTP/REST** (não usa Bluetooth)

### APIs REST Disponíveis

#### 1. `GET /` ou `GET /index.html`
- Retorna a página principal do sistema

#### 2. `POST /api/config`
- Configura temperaturas alvo
- **Body JSON:**
```json
{
  "heater": 25.5,
  "freezer": -5.0
}
```
- **Resposta:**
```json
{
  "status": "ok",
  "heater": 25.5,
  "freezer": -5.0
}
```

#### 3. `GET /api/status`
- Retorna status atual do sistema com dados simulados
- **Resposta:**
```json
{
  "heater": 26.2,
  "freezer": -4.8,
  "shaken": false
}
```
- Temperaturas variam ±2°C do valor configurado
- Estado "shaken" tem 20% de chance de ser true a cada consulta

#### 4. `POST /api/reset`
- Reseta o estado de "balançado/virado"
- **Resposta:**
```json
{
  "status": "ok"
}
```

## 🚀 Como Usar

### 1. Compilar o Projeto

```bash
# No PowerShell, navegue até a pasta do projeto
cd "d:\Carlo\Documentos\Raspberry Pi Pico 2 W\SDK\iBagPico2W"

# Execute a tarefa de compilação (ou use o VS Code)
# A tarefa "Compile Project" está configurada no workspace
```

### 2. Carregar no Pico 2 W

- Conecte o Pico 2 W via USB segurando o botão BOOTSEL
- Copie o arquivo `.uf2` da pasta `build` para o drive RPI-RP2
- Ou use a tarefa "Run Project" do VS Code

### 3. Conectar ao Access Point

1. **No seu dispositivo** (celular, tablet, notebook):
   - Busque redes WiFi disponíveis
   - Conecte-se à rede: **`iBag-Pico2W`**
   - Senha: **`ibag1234`**

2. **Acesse a interface web:**
   - Abra o navegador
   - Digite: **`http://192.168.4.1`**

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

No arquivo `iBagPico2W.c`, você pode alterar:

```c
#define AP_SSID "iBag-Pico2W"        // Nome da rede WiFi
#define AP_PASSWORD "ibag1234"       // Senha (mínimo 8 caracteres)
#define AP_CHANNEL 6                 // Canal WiFi (1-11)
```

**IP fixo do Pico:** `192.168.4.1`  
**Range DHCP:** `192.168.4.2` - `192.168.4.254`

## 📊 Simulação de Dados

### Temperaturas
- Valores gerados com variação aleatória de ±2°C (aquecedor) e ±1.5°C (congelador)
- Baseados nos valores configurados pelo usuário
- Atualizados a cada requisição de status

### Estado de Vibração
- 20% de chance de ser marcado como "balançado" em cada verificação
- Persiste até ser resetado manualmente
- Simula detector de movimento/acelerômetro

## 🛠️ Bibliotecas Utilizadas

- `pico_stdlib` - Funções básicas do SDK
- `pico_cyw43_arch_lwip_poll` - WiFi e stack TCP/IP
- `pico_lwip_http` - Servidor HTTP
- `pico_lwip_dhcpserver` - Servidor DHCP

## 🔍 Debug e Monitoramento

O Pico imprime informações via UART/USB:
- Status de inicialização
- Configurações do AP
- IP do servidor
- Mudanças de configuração
- Resetar de estados

Para ver as mensagens, conecte um monitor serial a 115200 baud.

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

## 💡 Próximas Melhorias Sugeridas

1. 📈 Adicionar gráficos de temperatura ao longo do tempo
2. 🔔 Notificações push quando detectar vibração
3. 📝 Log de eventos
4. ⚙️ Salvar configurações em flash
5. 🌡️ Integrar sensores reais (DS18B20, MPU6050)
6. 🔊 Alarme sonoro via buzzer
7. 🎯 Múltiplos perfis de temperatura
8. 📊 Dashboard com estatísticas

## 📞 Solução de Problemas

### Não consigo encontrar a rede WiFi
- Verifique se o LED do Pico está piscando
- Reinicie o Pico
- Verifique se está próximo o suficiente

### Não consigo acessar 192.168.4.1
- Confirme que está conectado ao WiFi correto
- Tente desligar dados móveis
- Limpe o cache do navegador
- Tente outro navegador

### As temperaturas não atualizam
- Verifique a conexão WiFi
- Recarregue a página
- Verifique o console do navegador (F12)

### Compilação falha
- Verifique se o SDK está instalado corretamente
- Confirme que está usando Pico 2 W (não Pico W normal)
- Limpe o build: delete a pasta `build` e recompile

## 📄 Licença

Este projeto é open source e pode ser modificado livremente para suas necessidades.

## 👨‍💻 Desenvolvido com

- Raspberry Pi Pico 2 W SDK
- lwIP (Lightweight IP stack)
- HTML5 + CSS3 + JavaScript
- C99

---

**Versão:** 1.0  
**Data:** Novembro 2025
