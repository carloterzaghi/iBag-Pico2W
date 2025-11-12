#include "simple_http_server.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "lwip/tcp.h"

extern const char html_content[];
extern float target_heater_temp;
extern float target_freezer_temp;
extern bool is_shaken;

// Funções auxiliares (declaradas em iBagPico2W.c)
extern float generate_random_temp(float base, float variance);
extern bool check_random_shake(void);

// Estrutura para rastrear estado da conexão
struct http_state {
    int total_sent;
    int total_length;
};

static err_t http_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    struct http_state *hs = (struct http_state *)arg;
    
    if (hs != NULL) {
        // Somar ACKs recebidos ao total
        hs->total_sent += len;
        
        printf("HTTP: ACK recebido - %d bytes confirmados (total: %d/%d)\n", 
               len, hs->total_sent, hs->total_length);
        
        // Fechar apenas quando tudo foi enviado E confirmado
        if (hs->total_sent >= hs->total_length) {
            printf("HTTP: Transferência completa! Fechando conexão.\n");
            tcp_arg(pcb, NULL);
            free(hs);
            tcp_close(pcb);
        }
    } else {
        printf("HTTP: Dados enviados (%d bytes), fechando.\n", len);
        tcp_close(pcb);
    }
    
    return ERR_OK;
}

static err_t http_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(pcb);
        return ERR_OK;
    }
    
    printf("\n>>> REQUISIÇÃO HTTP RECEBIDA! <<<\n");
    printf("Tamanho: %d bytes\n", p->tot_len);
    
    // Copiar requisição completa
    char request[512] = {0};
    int copy_len = (p->tot_len < 511) ? p->tot_len : 511;
    pbuf_copy_partial(p, request, copy_len, 0);
    request[copy_len] = '\0';
    
    printf("Request:\n%s\n", request);
    
    // Preparar resposta HTTP
    static char response[50000];
    int len = 0;
    
    // Detectar método e rota
    bool is_post = (strncmp(request, "POST", 4) == 0);
    bool is_get = (strncmp(request, "GET", 3) == 0);
    
    // Encontrar a URI (entre o método e HTTP/1.x)
    char *uri_start = strchr(request, ' ');
    if (uri_start) {
        uri_start++; // pular o espaço
        char *uri_end = strchr(uri_start, ' ');
        if (uri_end) {
            int uri_len = uri_end - uri_start;
            char uri[128] = {0};
            if (uri_len < 127) {
                memcpy(uri, uri_start, uri_len);
                uri[uri_len] = '\0';
                printf("URI detectada: %s\n", uri);
                
                // Roteamento
                if (is_get && (strcmp(uri, "/") == 0 || strcmp(uri, "/index.html") == 0)) {
                    // Página principal
                    len = snprintf(response, sizeof(response),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html; charset=UTF-8\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "%s", html_content);
                    printf("Servindo página principal (%d bytes)\n", len);
                }
                else if (is_get && strcmp(uri, "/api/status") == 0) {
                    // API de status
                    float current_heater = generate_random_temp(target_heater_temp, 2.0f);
                    float current_freezer = generate_random_temp(target_freezer_temp, 1.5f);
                    
                    if (!is_shaken) {
                        is_shaken = check_random_shake();
                    }
                    
                    char json[256];
                    snprintf(json, sizeof(json),
                            "{\"heater\":%.1f,\"freezer\":%.1f,\"shaken\":%s}",
                            current_heater, current_freezer, is_shaken ? "true" : "false");
                    
                    len = snprintf(response, sizeof(response),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "%s", json);
                    printf("API Status: %s\n", json);
                }
                else if (is_post && strcmp(uri, "/api/config") == 0) {
                    // Extrair corpo do POST (JSON está após \r\n\r\n)
                    char *body = strstr(request, "\r\n\r\n");
                    if (body) {
                        body += 4; // pular \r\n\r\n
                        printf("POST body: %s\n", body);
                        
                        // Parse simples do JSON
                        char *heater_str = strstr(body, "\"heater\":");
                        char *freezer_str = strstr(body, "\"freezer\":");
                        
                        if (heater_str) {
                            heater_str += 9;
                            target_heater_temp = atof(heater_str);
                            printf("Nova temperatura aquecedor: %.1f C\n", target_heater_temp);
                        }
                        
                        if (freezer_str) {
                            freezer_str += 10;
                            target_freezer_temp = atof(freezer_str);
                            printf("Nova temperatura congelador: %.1f C\n", target_freezer_temp);
                        }
                    }
                    
                    char json[128];
                    snprintf(json, sizeof(json),
                            "{\"status\":\"ok\",\"heater\":%.1f,\"freezer\":%.1f}",
                            target_heater_temp, target_freezer_temp);
                    
                    len = snprintf(response, sizeof(response),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "%s", json);
                    printf("Config atualizada: %s\n", json);
                }
                else if (is_post && strcmp(uri, "/api/reset") == 0) {
                    is_shaken = false;
                    printf("Estado balançado resetado\n");
                    
                    len = snprintf(response, sizeof(response),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "{\"status\":\"ok\"}");
                }
                else {
                    // 404 Not Found
                    len = snprintf(response, sizeof(response),
                        "HTTP/1.1 404 Not Found\r\n"
                        "Content-Type: text/plain\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "404 - Not Found");
                    printf("404: %s\n", uri);
                }
            }
        }
    }
    
    // Enviar resposta em chunks para evitar overflow do buffer TCP
    if (len > 0) {
        // Criar estrutura de estado para rastrear envio
        struct http_state *hs = (struct http_state *)malloc(sizeof(struct http_state));
        if (hs == NULL) {
            printf("ERRO: Falha ao alocar memória para http_state\n");
            tcp_close(pcb);
            pbuf_free(p);
            return ERR_MEM;
        }
        
        hs->total_length = len;
        hs->total_sent = 0;
        
        // Associar estado à conexão
        tcp_arg(pcb, hs);
        
        int sent = 0;
        int chunk_size = 1024; // 1KB por vez
        
        while (sent < len) {
            int to_send = (len - sent) < chunk_size ? (len - sent) : chunk_size;
            
            // Verificar espaço disponível no buffer TCP
            int available = tcp_sndbuf(pcb);
            if (available < to_send) {
                to_send = available;
            }
            
            if (to_send > 0) {
                err_t write_err = tcp_write(pcb, response + sent, to_send, TCP_WRITE_FLAG_COPY);
                if (write_err == ERR_OK) {
                    sent += to_send;
                    // NÃO atualizar hs->total_sent aqui - será feito no callback http_sent()
                } else {
                    printf("ERRO ao enviar chunk: %d\n", write_err);
                    break;
                }
            } else {
                // Buffer cheio, forçar envio e aguardar
                tcp_output(pcb);
                sleep_ms(10);
            }
        }
        
        // Forçar envio de todos os dados pendentes
        tcp_output(pcb);
        
        printf("Total enfileirado: %d de %d bytes\n", sent, len);
    } else {
        tcp_close(pcb);
    }
    
    // Liberar buffer recebido
    pbuf_free(p);
    
    return ERR_OK;
}

static err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    printf("\n╔════════════════════════════════════╗\n");
    printf("║  CLIENTE CONECTADO!               ║\n");
    printf("║  Nova conexão TCP na porta 8000    ║\n");
    printf("╚════════════════════════════════════╝\n\n");
    
    tcp_recv(newpcb, http_recv);
    tcp_sent(newpcb, http_sent);
    tcp_err(newpcb, NULL);
    
    return ERR_OK;
}

void simple_http_server_init(void) {
    struct tcp_pcb *pcb = tcp_new();
    
    if (pcb == NULL) {
        printf("ERRO: Falha ao criar TCP PCB\n");
        return;
    }
    
    err_t err = tcp_bind(pcb, IP_ADDR_ANY, 8000);
    if (err != ERR_OK) {
        printf("ERRO: Falha ao fazer bind na porta 8000 (erro %d)\n", err);
        tcp_close(pcb);
        return;
    }
    
    pcb = tcp_listen(pcb);
    if (pcb == NULL) {
        printf("ERRO: Falha ao colocar em modo listen\n");
        return;
    }
    
    tcp_accept(pcb, http_accept);
    
    printf("✅ Servidor HTTP simples escutando na porta 8000\n");
    printf("   Servidor TCP iniciado com sucesso!\n");
}
