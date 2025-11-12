#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/dhcp.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"
#include "simple_http_server.h"
#include "dhcp_server.h"

// Configurações do Access Point
#define AP_SSID "iBag-Pico2W"
#define AP_PASSWORD "ibag12345678"
#define AP_CHANNEL 1

// Configurações de temperatura (valores configuráveis - NÃO-STATIC para acesso externo)
float target_heater_temp = 25.0f;
float target_freezer_temp = -5.0f;
bool is_shaken = false;

// Função para gerar temperatura aleatória com variação (NÃO-STATIC para acesso externo)
float generate_random_temp(float target, float variation) {
    float random_offset = ((float)rand() / RAND_MAX) * variation * 2.0f - variation;
    return target + random_offset;
}

// Função para gerar estado de "virou" aleatoriamente (20% de chance)
bool check_random_shake(void) {
    return (rand() % 100) < 20;  // 20% chance de ter sido balançado
}

// Conteúdo HTML embutido (não-static para ser acessível externamente)
const char html_content[] = 
"<!DOCTYPE html>\n"
"<html lang=\"pt-BR\">\n"
"<head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"    <title>iBag - Controle de Temperatura</title>\n"
"    <style>\n"
"        * { margin: 0; padding: 0; box-sizing: border-box; }\n"
"        :root {\n"
"            --primary-color: #2563eb;\n"
"            --success-color: #10b981;\n"
"            --warning-color: #f59e0b;\n"
"            --danger-color: #ef4444;\n"
"            --bg-color: #f8fafc;\n"
"            --card-bg: #ffffff;\n"
"            --text-primary: #1e293b;\n"
"            --text-secondary: #64748b;\n"
"            --border-color: #e2e8f0;\n"
"        }\n"
"        body {\n"
"            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;\n"
"            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n"
"            min-height: 100vh;\n"
"            padding: 20px;\n"
"            color: var(--text-primary);\n"
"        }\n"
"        .container { max-width: 800px; margin: 0 auto; }\n"
"        header {\n"
"            text-align: center;\n"
"            color: white;\n"
"            margin-bottom: 30px;\n"
"            padding: 20px;\n"
"        }\n"
"        header h1 { font-size: 3rem; margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0,0,0,0.2); }\n"
"        header p { font-size: 1.2rem; opacity: 0.9; }\n"
"        section {\n"
"            background: var(--card-bg);\n"
"            border-radius: 16px;\n"
"            padding: 24px;\n"
"            margin-bottom: 20px;\n"
"            box-shadow: 0 10px 15px -3px rgba(0,0,0,0.1);\n"
"        }\n"
"        section h2 { font-size: 1.5rem; margin-bottom: 20px; }\n"
"        .status-badge {\n"
"            padding: 8px 16px;\n"
"            border-radius: 20px;\n"
"            font-weight: 600;\n"
"            background-color: #d1fae5;\n"
"            color: var(--success-color);\n"
"            display: inline-block;\n"
"            margin-bottom: 16px;\n"
"        }\n"
"        .config-group { display: flex; flex-direction: column; gap: 20px; margin-bottom: 24px; }\n"
"        .input-group { display: flex; flex-direction: column; gap: 8px; }\n"
"        .input-group label { font-weight: 600; display: flex; align-items: center; gap: 8px; }\n"
"        .input-wrapper {\n"
"            display: flex;\n"
"            align-items: center;\n"
"            gap: 8px;\n"
"            background: var(--bg-color);\n"
"            border: 2px solid var(--border-color);\n"
"            border-radius: 8px;\n"
"            padding: 4px 12px;\n"
"        }\n"
"        .input-wrapper input {\n"
"            flex: 1;\n"
"            border: none;\n"
"            background: transparent;\n"
"            font-size: 1.1rem;\n"
"            padding: 8px;\n"
"            outline: none;\n"
"        }\n"
"        .unit { font-weight: 600; color: var(--text-secondary); }\n"
"        .btn {\n"
"            display: inline-flex;\n"
"            align-items: center;\n"
"            justify-content: center;\n"
"            gap: 8px;\n"
"            padding: 12px 24px;\n"
"            border: none;\n"
"            border-radius: 8px;\n"
"            font-size: 1rem;\n"
"            font-weight: 600;\n"
"            cursor: pointer;\n"
"            width: 100%;\n"
"            transition: all 0.3s ease;\n"
"        }\n"
"        .btn:hover { transform: translateY(-2px); }\n"
"        .btn-success { background-color: var(--success-color); color: white; }\n"
"        .btn-info { background-color: #06b6d4; color: white; }\n"
"        .btn-warning { background-color: var(--warning-color); color: white; margin-top: 12px; }\n"
"        .status-display { margin-top: 20px; }\n"
"        .status-card {\n"
"            background: var(--bg-color);\n"
"            border-radius: 12px;\n"
"            padding: 20px;\n"
"            margin-bottom: 16px;\n"
"        }\n"
"        .temp-display { display: flex; flex-direction: column; gap: 12px; }\n"
"        .temp-item {\n"
"            display: flex;\n"
"            align-items: center;\n"
"            gap: 12px;\n"
"            padding: 12px;\n"
"            background: white;\n"
"            border-radius: 8px;\n"
"            border: 2px solid var(--border-color);\n"
"        }\n"
"        .temp-item .value {\n"
"            margin-left: auto;\n"
"            font-size: 1.3rem;\n"
"            font-weight: 700;\n"
"            color: var(--primary-color);\n"
"        }\n"
"        .shake-status {\n"
"            display: flex;\n"
"            align-items: center;\n"
"            gap: 12px;\n"
"            padding: 16px;\n"
"            background: white;\n"
"            border-radius: 8px;\n"
"            border: 2px solid var(--border-color);\n"
"            font-size: 1.1rem;\n"
"            font-weight: 600;\n"
"        }\n"
"        .shake-status.shaken { border-color: var(--warning-color); background-color: #fef3c7; }\n"
"        .shake-status.stable { border-color: var(--success-color); background-color: #d1fae5; }\n"
"        .hidden { display: none !important; }\n"
"        .popup {\n"
"            position: fixed;\n"
"            top: 0; left: 0;\n"
"            width: 100%; height: 100%;\n"
"            background-color: rgba(0,0,0,0.5);\n"
"            display: flex;\n"
"            align-items: center;\n"
"            justify-content: center;\n"
"            z-index: 1000;\n"
"        }\n"
"        .popup-content {\n"
"            background: white;\n"
"            border-radius: 16px;\n"
"            padding: 32px;\n"
"            text-align: center;\n"
"            max-width: 400px;\n"
"        }\n"
"        .popup-icon { font-size: 4rem; margin-bottom: 16px; }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <div class=\"container\">\n"
"        <header>\n"
"            <h1>🎒 iBag</h1>\n"
"            <p>Sistema de Controle de Temperatura</p>\n"
"        </header>\n"
"        <section>\n"
"            <h2>Configurações de Temperatura</h2>\n"
"            <div class=\"config-group\">\n"
"                <div class=\"input-group\">\n"
"                    <label><span>🔥</span> Temperatura do Aquecedor</label>\n"
"                    <div class=\"input-wrapper\">\n"
"                        <input type=\"number\" id=\"heaterTemp\" min=\"-10\" max=\"100\" step=\"0.5\" value=\"25\">\n"
"                        <span class=\"unit\">°C</span>\n"
"                    </div>\n"
"                </div>\n"
"                <div class=\"input-group\">\n"
"                    <label><span>❄️</span> Temperatura do Congelador</label>\n"
"                    <div class=\"input-wrapper\">\n"
"                        <input type=\"number\" id=\"freezerTemp\" min=\"-50\" max=\"20\" step=\"0.5\" value=\"-5\">\n"
"                        <span class=\"unit\">°C</span>\n"
"                    </div>\n"
"                </div>\n"
"            </div>\n"
"            <button class=\"btn btn-success\" onclick=\"sendConfig()\">📤 Enviar Configurações</button>\n"
"        </section>\n"
"        <section>\n"
"            <h2>Status do Sistema</h2>\n"
"            <button class=\"btn btn-info\" onclick=\"checkStatus()\">🔍 Verificar Status da Comida</button>\n"
"            <div id=\"statusDisplay\" class=\"status-display hidden\">\n"
"                <div class=\"status-card\">\n"
"                    <h3>Temperaturas Atuais</h3>\n"
"                    <div class=\"temp-display\">\n"
"                        <div class=\"temp-item\">\n"
"                            <span>🔥</span>\n"
"                            <span>Aquecedor:</span>\n"
"                            <span id=\"currentHot\" class=\"value\">-- °C</span>\n"
"                        </div>\n"
"                        <div class=\"temp-item\">\n"
"                            <span>❄️</span>\n"
"                            <span>Congelador:</span>\n"
"                            <span id=\"currentCold\" class=\"value\">-- °C</span>\n"
"                        </div>\n"
"                    </div>\n"
"                </div>\n"
"                <div class=\"status-card\">\n"
"                    <h3>Estado da Comida</h3>\n"
"                    <div id=\"shakeStatus\" class=\"shake-status\"></div>\n"
"                    <button id=\"resetShakeBtn\" class=\"btn btn-warning hidden\" onclick=\"resetShake()\">🔄 Resetar Estado</button>\n"
"                </div>\n"
"            </div>\n"
"        </section>\n"
"    </div>\n"
"    <div id=\"popup\" class=\"popup hidden\">\n"
"        <div class=\"popup-content\">\n"
"            <span class=\"popup-icon\" id=\"popupIcon\">✅</span>\n"
"            <h3 id=\"popupTitle\">Sucesso!</h3>\n"
"            <p id=\"popupMessage\">Operação concluída</p>\n"
"            <button class=\"btn btn-success\" onclick=\"closePopup()\">OK</button>\n"
"        </div>\n"
"    </div>\n"
"    <script>\n"
"        function showPopup(icon, title, msg) {\n"
"            document.getElementById('popupIcon').textContent = icon;\n"
"            document.getElementById('popupTitle').textContent = title;\n"
"            document.getElementById('popupMessage').textContent = msg;\n"
"            document.getElementById('popup').classList.remove('hidden');\n"
"        }\n"
"        function closePopup() {\n"
"            document.getElementById('popup').classList.add('hidden');\n"
"        }\n"
"        async function sendConfig() {\n"
"            const heater = document.getElementById('heaterTemp').value;\n"
"            const freezer = document.getElementById('freezerTemp').value;\n"
"            try {\n"
"                const controller = new AbortController();\n"
"                const timeoutId = setTimeout(() => controller.abort(), 5000);\n"
"                const resp = await fetch('/api/config', {\n"
"                    method: 'POST',\n"
"                    headers: {'Content-Type': 'application/json'},\n"
"                    body: JSON.stringify({heater: parseFloat(heater), freezer: parseFloat(freezer)}),\n"
"                    signal: controller.signal\n"
"                });\n"
"                clearTimeout(timeoutId);\n"
"                const data = await resp.json();\n"
"                if(data.status === 'ok') {\n"
"                    showPopup('✅', 'Sucesso!', 'Configurações atualizadas!');\n"
"                } else {\n"
"                    showPopup('❌', 'Erro', 'Falha ao atualizar');\n"
"                }\n"
"            } catch(e) {\n"
"                console.log('Erro:', e);\n"
"                showPopup('❌', 'Erro', 'Falha na comunicação: ' + e.message);\n"
"            }\n"
"        }\n"
"        async function checkStatus() {\n"
"            try {\n"
"                const controller = new AbortController();\n"
"                const timeoutId = setTimeout(() => controller.abort(), 5000);\n"
"                const resp = await fetch('/api/status', {\n"
"                    signal: controller.signal\n"
"                });\n"
"                clearTimeout(timeoutId);\n"
"                const data = await resp.json();\n"
"                document.getElementById('currentHot').textContent = data.heater.toFixed(1) + ' °C';\n"
"                document.getElementById('currentCold').textContent = data.freezer.toFixed(1) + ' °C';\n"
"                const shakeDiv = document.getElementById('shakeStatus');\n"
"                const resetBtn = document.getElementById('resetShakeBtn');\n"
"                if(data.shaken) {\n"
"                    shakeDiv.className = 'shake-status shaken';\n"
"                    shakeDiv.textContent = '⚠️ Comida foi Balançada!';\n"
"                    resetBtn.classList.remove('hidden');\n"
"                } else {\n"
"                    shakeDiv.className = 'shake-status stable';\n"
"                    shakeDiv.textContent = '✅ Comida Estável';\n"
"                    resetBtn.classList.add('hidden');\n"
"                }\n"
"                document.getElementById('statusDisplay').classList.remove('hidden');\n"
"            } catch(e) {\n"
"                console.log('Erro ao verificar status:', e);\n"
"                showPopup('❌', 'Erro', 'Falha ao verificar status: ' + e.message);\n"
"            }\n"
"        }\n"
"        async function resetShake() {\n"
"            try {\n"
"                const controller = new AbortController();\n"
"                const timeoutId = setTimeout(() => controller.abort(), 5000);\n"
"                const resp = await fetch('/api/reset', {\n"
"                    method: 'POST',\n"
"                    signal: controller.signal\n"
"                });\n"
"                clearTimeout(timeoutId);\n"
"                const data = await resp.json();\n"
"                if(data.status === 'ok') {\n"
"                    showPopup('✅', 'Resetado!', 'Estado balançado foi resetado');\n"
"                    await checkStatus();\n"
"                }\n"
"            } catch(e) {\n"
"                console.log('Erro ao resetar:', e);\n"
"                showPopup('❌', 'Erro', 'Falha ao resetar: ' + e.message);\n"
"            }\n"
"        }\n"
"        // Auto-atualizar status a cada 5 segundos se o display estiver visível\n"
"        setInterval(() => {\n"
"            if(!document.getElementById('statusDisplay').classList.contains('hidden')) {\n"
"                checkStatus();\n"
"            }\n"
"        }, 5000);\n"
"    </script>\n"
"</body>\n"
"</html>";

// Handler para a página principal
const char* http_get_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    return "/index.html";
}

// Handler para API de configuração
const char* http_api_config_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    return "/api_config.json";
}

// Handler para API de status
const char* http_api_status_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    return "/api_status.json";
}

// Handler para API de reset
const char* http_api_reset_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    return "/api_reset.json";
}

// Tabela de handlers CGI
static const tCGI cgi_handlers[] = {
    {"/", http_get_handler},
    {"/api/config", http_api_config_handler},
    {"/api/status", http_api_status_handler},
    {"/api/reset", http_api_reset_handler},
};

// Handler para POST
err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                      u16_t http_request_len, int content_len, char *response_uri,
                      u16_t response_uri_len, u8_t *post_auto_wnd) {
    if (strncmp(uri, "/api/config", 11) == 0) {
        return ERR_OK;
    }
    if (strncmp(uri, "/api/reset", 10) == 0) {
        return ERR_OK;
    }
    return ERR_VAL;
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
    // Processar dados JSON recebidos
    char buffer[256];
    if (p->len < sizeof(buffer)) {
        pbuf_copy_partial(p, buffer, p->len, 0);
        buffer[p->len] = '\0';
        
        // Parse simples do JSON para extrair valores
        char *heater_str = strstr(buffer, "\"heater\":");
        char *freezer_str = strstr(buffer, "\"freezer\":");
        
        if (heater_str) {
            heater_str += 9; // pula "heater":
            target_heater_temp = atof(heater_str);
            printf("Nova temperatura aquecedor: %.1f C\n", target_heater_temp);
        }
        
        if (freezer_str) {
            freezer_str += 10; // pula "freezer":
            target_freezer_temp = atof(freezer_str);
            printf("Nova temperatura congelador: %.1f C\n", target_freezer_temp);
        }
    }
    
    pbuf_free(p);
    return ERR_OK;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {
    snprintf(response_uri, response_uri_len, "/api_config.json");
}

// Handler para fornecer conteúdo dos arquivos
int fs_open_custom(struct fs_file *file, const char *name) {
    static bool first_request = true;
    
    if (first_request) {
        printf("\n");
        printf("╔════════════════════════════════════╗\n");
        printf("║  CLIENTE CONECTADO AO WIFI!       ║\n");
        printf("║  Primeira requisição HTTP recebida ║\n");
        printf("╚════════════════════════════════════╝\n");
        printf("\n");
        first_request = false;
    }
    
    printf(">>> Requisição HTTP: %s\n", name);
    
    // Servir a página principal tanto para / quanto para /index.html
    if (strcmp(name, "/") == 0 || strcmp(name, "/index.html") == 0) {
        file->data = html_content;
        file->len = sizeof(html_content) - 1;
        file->index = 0;
        file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;
        printf(">>> Servindo página principal (%d bytes)\n", file->len);
        return 1;
    }
    else if (strcmp(name, "/api_config.json") == 0) {
        static char json_response[128];
        snprintf(json_response, sizeof(json_response),
                "{\"status\":\"ok\",\"heater\":%.1f,\"freezer\":%.1f}",
                target_heater_temp, target_freezer_temp);
        file->data = json_response;
        file->len = strlen(json_response);
        file->index = 0;
        file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;
        return 1;
    }
    else if (strcmp(name, "/api_status.json") == 0) {
        static char json_response[256];
        
        // Gerar temperaturas atuais com pequena variação
        float current_heater = generate_random_temp(target_heater_temp, 2.0f);
        float current_freezer = generate_random_temp(target_freezer_temp, 1.5f);
        
        // Verificar aleatoriamente se foi balançado
        if (!is_shaken) {
            is_shaken = check_random_shake();
        }
        
        snprintf(json_response, sizeof(json_response),
                "{\"heater\":%.1f,\"freezer\":%.1f,\"shaken\":%s}",
                current_heater, current_freezer, is_shaken ? "true" : "false");
        
        file->data = json_response;
        file->len = strlen(json_response);
        file->index = 0;
        file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;
        return 1;
    }
    else if (strcmp(name, "/api_reset.json") == 0) {
        is_shaken = false;
        printf("Estado balançado resetado\n");
        
        static char json_response[64];
        snprintf(json_response, sizeof(json_response), "{\"status\":\"ok\"}");
        file->data = json_response;
        file->len = strlen(json_response);
        file->index = 0;
        file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;
        return 1;
    }
    
    printf("Arquivo não encontrado: %s\n", name);
    return 0;
}

int fs_read_custom(struct fs_file *file, char *buffer, int count) {
    // Não é necessário para nosso caso, pois os dados já estão disponíveis via file->data
    return FS_READ_EOF;
}

void fs_close_custom(struct fs_file *file) {
    // Não precisa fazer nada, dados são estáticos
}

// Variável para rastrear clientes conectados
static int connected_clients = 0;

// Callback para status da interface de rede
void netif_status_callback(struct netif *netif) {
    if (netif_is_up(netif) && netif_is_link_up(netif)) {
        printf("\n>>> INTERFACE DE REDE ATIVA <<<\n");
        printf("IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
    }
}

int main() {
    stdio_init_all();
    
    // Inicializar gerador de números aleatórios
    srand(time(NULL));
    
    printf("\n=================================\n");
    printf("iBag - Pico 2 W Access Point\n");
    printf("=================================\n\n");
    
    // Inicializar Wi-Fi em modo AP
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar cyw43_arch\n");
        return 1;
    }
    
    // Habilitar modo Access Point
    cyw43_arch_enable_ap_mode(AP_SSID, AP_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
    
    printf("Access Point iniciado!\n");
    printf("SSID: %s\n", AP_SSID);
    printf("Senha: %s\n", AP_PASSWORD);
    
    // Obter interface de rede
    struct netif *n = &cyw43_state.netif[CYW43_ITF_AP];
    
    // Configurar IP manualmente
    ip4_addr_t ipaddr, netmask, gw;
    IP4_ADDR(&ipaddr, 192, 168, 4, 1);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 4, 1);
    netif_set_addr(n, &ipaddr, &netmask, &gw);
    
    printf("IP configurado: %s\n", ip4addr_ntoa(netif_ip4_addr(n)));
    printf("Netmask: %s\n", ip4addr_ntoa(netif_ip4_netmask(n)));
    printf("Gateway: %s\n\n", ip4addr_ntoa(netif_ip4_gw(n)));
    
    // Configurar callback de status da interface
    netif_set_status_callback(n, netif_status_callback);
    
    // Levantar a interface
    netif_set_up(n);
    netif_set_link_up(n);
    
    printf("Interface de rede ativada\n");
    
    // Aguardar AP estar completamente pronto
    sleep_ms(3000);
    printf("\n");
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  AP PRONTO PARA ACEITAR CONEXÕES WiFi!           ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("⚠️  PROBLEMA CONHECIDO: DHCP pode não funcionar\n");
    printf("\n");
    printf("📱 SOLUÇÃO - Configure IP MANUALMENTE no celular:\n");
    printf("   1. Conecte ao WiFi: iBag-Pico2W\n");
    printf("   2. Se ficar em loop, vá em Configurações Avançadas\n");
    printf("   3. Mude de DHCP para IP ESTÁTICO\n");
    printf("   4. Configure:\n");
    printf("      • IP: 192.168.4.2\n");
    printf("      • Máscara: 255.255.255.0\n");
    printf("      • Gateway: 192.168.4.1\n");
    printf("      • DNS: 192.168.4.1\n");
    printf("   5. Salve e reconecte\n");
    printf("   6. Abra o navegador: http://192.168.4.1:8000\n");
    printf("\n");
    
    // Inicializar servidor DHCP (igual ao MicroPython!)
    printf("Inicializando servidor DHCP...\n");
    dhcp_server_init();
    printf("\n");
    
    // Inicializar nosso servidor HTTP simples
    printf("Inicializando servidor HTTP customizado...\n");
    simple_http_server_init();
    
    // Verificar se a pilha TCP está funcionando
    printf("Interface de rede UP: %s\n", netif_is_up(n) ? "SIM" : "NÃO");
    printf("Link UP: %s\n", netif_is_link_up(n) ? "SIM" : "NÃO");
    
    printf("\n=================================\n");
    printf("🎉 SISTEMA COMPLETO ATIVO!  🎉\n");
    printf("=================================\n");
    printf("📡 DHCP Server: Porta 67\n");
    printf("🌐 HTTP Server: Porta 8000\n");
    printf("🔗 Acesse: http://192.168.4.1:8000\n");
    printf("=================================\n\n");
    printf("✅ Agora você NÃO precisa configurar IP manual!\n");
    printf("   O dispositivo receberá IP automaticamente via DHCP\n\n");
    printf("(Se não funcionar, verifique se o dispositivo obteve IP via DHCP)\n\n");
    
    // LED piscando para indicar que está funcionando
    uint32_t led_counter = 0;
    uint32_t status_print_counter = 0;
    
    while (true) {
        // Processar eventos de rede constantemente
        cyw43_arch_poll();
        
        // LED piscando (controle por contador ao invés de sleep)
        if (led_counter % 10000 == 0) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, (led_counter / 10000) % 2);
        }
        led_counter++;
        
        // Print de status a cada 5 segundos
        if (status_print_counter % 5000 == 0 && status_print_counter > 0) {
            printf("[STATUS] Sistema rodando... Aguardando conexões...\n");
        }
        status_print_counter++;
        
        // Pequeno delay para não sobrecarregar a CPU
        sleep_ms(1);
    }
    
    cyw43_arch_deinit();
    return 0;
}
