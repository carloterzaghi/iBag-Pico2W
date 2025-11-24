#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
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
#include "mpu6050.h"
#include "web_content.h"  // Conteúdo HTML da interface web

// Configurações do Access Point
#define AP_SSID "iBag-Pico2W"
#define AP_PASSWORD "ibag12345678"
#define AP_CHANNEL 1

// Configurações dos sensores LM35
// GPIO 26 = ADC0, GPIO 27 = ADC1
#define ADC_HEATER 1    // ADC1 - GPIO 27 (aquecedor)
#define ADC_FREEZER 0   // ADC0 - GPIO 26 (congelador)

// Configurações de temperatura (valores configuráveis - NÃO-STATIC para acesso externo)
float target_heater_temp = 25.0f;   // Mantido para referência/configuração futura
float target_freezer_temp = -5.0f;  // Mantido para referência/configuração futura
bool is_shaken = false;

// Função para inicializar o ADC
void init_adc_sensors(void) {
    adc_init();
    adc_gpio_init(26);  // GPIO 26 como entrada analógica (ADC0)
    adc_gpio_init(27);  // GPIO 27 como entrada analógica (ADC1)
    printf("Sensores LM35 inicializados:\n");
    printf("  - GPIO 27 (ADC1): Aquecedor\n");
    printf("  - GPIO 26 (ADC0): Congelador\n\n");
}

// Função para ler temperatura do LM35 (retorna em Celsius)
// LM35: 10mV/°C, ADC: 12-bit (0-4095), Vref: 3.3V
// Temp (°C) = (ADC_value * 3.3 / 4095) / 0.01
float read_lm35_temp(uint8_t adc_channel) {
    adc_select_input(adc_channel);
    uint16_t adc_value = adc_read();
    float voltage = (adc_value * 3.3f) / 4095.0f;
    float temperature = voltage / 0.01f;  // LM35: 10mV/°C
    return temperature;
}

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

// Variável global para rastrear qual POST está sendo processado
static char current_post_uri[32] = "";

// Handler para POST
err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                      u16_t http_request_len, int content_len, char *response_uri,
                      u16_t response_uri_len, u8_t *post_auto_wnd) {
    if (strncmp(uri, "/api/config", 11) == 0) {
        strncpy(current_post_uri, uri, sizeof(current_post_uri) - 1);
        return ERR_OK;
    }
    if (strncmp(uri, "/api/reset", 10) == 0) {
        strncpy(current_post_uri, uri, sizeof(current_post_uri) - 1);
        return ERR_OK;
    }
    return ERR_VAL;
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
    // Processar dados JSON recebidos (apenas para config)
    if (strncmp(current_post_uri, "/api/config", 11) == 0) {
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
    }
    
    pbuf_free(p);
    return ERR_OK;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len) {
    // Redirecionar para a URI correta baseado no POST recebido
    if (strncmp(current_post_uri, "/api/reset", 10) == 0) {
        snprintf(response_uri, response_uri_len, "/api_reset.json");
    } else {
        snprintf(response_uri, response_uri_len, "/api_config.json");
    }
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
        
        // Ler temperaturas reais dos sensores LM35
        float current_heater = read_lm35_temp(ADC_HEATER);
        float current_freezer = read_lm35_temp(ADC_FREEZER);
        
        // Verificar virada brusca usando MPU6050
        if (!is_shaken) {
            is_shaken = mpu6050_detect_shake();
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
        printf("\n🔄 RESETANDO ESTADO E RECALIBRANDO...\n");
        
        // Reset do estado
        is_shaken = false;
        
        printf("Estado balançado resetado\n");
        printf("⏱️  Iniciando calibração do MPU6050...\n");
        printf("    NÃO MOVA O DISPOSITIVO por 10 segundos!\n\n");
        
        // IMPORTANTE: Resetar e calibrar ANTES de responder HTTP
        mpu6050_reset_shake_detection();
        
        // Aguardar calibração completar (10 segundos) ANTES de responder
        for (int i = 10; i > 0; i--) {
            printf("    Calibrando... %d segundos restantes\n", i);
            sleep_ms(1000);
            mpu6050_update_calibration();
            cyw43_arch_poll();  // Manter conexão TCP viva durante calibração
        }
        
        printf("\n✅ Calibração completa! Sistema pronto para detectar movimento.\n\n");
        
        // Agora sim, responder HTTP
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
    
    // Inicializar sensores LM35
    init_adc_sensors();
    
    // Inicializar MPU6050 (acelerômetro/giroscópio)
    if (!mpu6050_init()) {
        printf("ERRO: Falha ao inicializar MPU6050!\n");
        printf("Verifique as conexões I2C (SDA=GPIO20, SCL=GPIO21)\n");
        return 1;
    }
    
    // Iniciar calibração automática do MPU6050
    printf("\n⏱️  Iniciando calibração do MPU6050...\n");
    printf("    NÃO MOVA O DISPOSITIVO por 10 segundos!\n\n");
    mpu6050_reset_shake_detection();
    
    // Aguardar calibração completar (10 segundos)
    for (int i = 10; i > 0; i--) {
        printf("    Calibrando... %d segundos restantes\n", i);
        sleep_ms(1000);
        mpu6050_update_calibration();
    }
    printf("\n✅ Calibração completa! Sistema pronto para detectar movimento.\n\n");
    
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
    printf(" SISTEMA COMPLETO ATIVO! \n");
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
    uint32_t mpu_log_counter = 0;
    
    while (true) {
        // Processar eventos de rede constantemente
        cyw43_arch_poll();
        
        // Atualizar processo de calibração do MPU6050
        mpu6050_update_calibration();
        
        // Verificar shake do MPU6050 periodicamente (a cada ~100ms)
        if (mpu_log_counter % 100 == 0) {
            mpu6050_detect_shake();
        }
        mpu_log_counter++;
        
        // LED piscando (controle por contador ao invés de sleep)
        if (led_counter % 10000 == 0) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, (led_counter / 10000) % 2);
        }
        led_counter++;
        
        // Print de status a cada 5 segundos
        if (status_print_counter % 5000 == 0 && status_print_counter > 0) {
            printf("[STATUS] Sistema rodando... Aguardando conexões... | Shaken: %s\n", 
                   is_shaken ? "SIM" : "NAO");
        }
        status_print_counter++;
        
        // Pequeno delay para não sobrecarregar a CPU
        sleep_ms(1);
    }
    
    cyw43_arch_deinit();
    return 0;
}
