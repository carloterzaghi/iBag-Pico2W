#include "dhcp_server.h"
#include <string.h>
#include <stdio.h>
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_MAGIC_COOKIE 0x63825363

// Pool de IPs para distribuir
static uint8_t next_ip = 2; // Começa em 192.168.4.2

struct dhcp_msg {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint32_t magic_cookie;
    uint8_t options[308];
};

static void dhcp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    printf("\n>>> DHCP REQUEST RECEBIDO! <<<\n");
    printf("De: %s:%d\n", ipaddr_ntoa(addr), port);
    printf("Tamanho do pacote: %d bytes (esperado >= 240)\n", p->tot_len);
    
    if (p->tot_len < 240) {  // Tamanho mínimo DHCP
        printf("Pacote DHCP muito pequeno (recebido: %d)\n", p->tot_len);
        pbuf_free(p);
        return;
    }
    
    // Copiar dados do pbuf para buffer contínuo
    static uint8_t dhcp_buffer[512];
    pbuf_copy_partial(p, dhcp_buffer, p->tot_len, 0);
    struct dhcp_msg *request = (struct dhcp_msg *)dhcp_buffer;
    
    printf("XID: 0x%08x\n", request->xid);
    printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", 
           request->chaddr[0], request->chaddr[1], request->chaddr[2],
           request->chaddr[3], request->chaddr[4], request->chaddr[5]);
    
    // Detectar tipo de mensagem DHCP (opção 53)
    uint8_t msg_type = 0;
    uint8_t *opt_ptr = request->options;
    while (*opt_ptr != 255 && (opt_ptr - request->options) < 308) {
        if (*opt_ptr == 53 && *(opt_ptr+1) == 1) {
            msg_type = *(opt_ptr+2);
            break;
        }
        opt_ptr += 2 + *(opt_ptr+1);
    }
    
    printf("Tipo de mensagem DHCP: %d ", msg_type);
    if (msg_type == 1) printf("(DISCOVER)\n");
    else if (msg_type == 3) printf("(REQUEST)\n");
    else printf("(OUTRO)\n");
    
    // Ignora se não for DISCOVER ou REQUEST
    if (msg_type != 1 && msg_type != 3) {
        printf("Mensagem ignorada\n");
        pbuf_free(p);
        return;
    }
    
    // Cria resposta DHCP OFFER/ACK
    struct pbuf *response_buf = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct dhcp_msg), PBUF_RAM);
    if (!response_buf) {
        printf("Erro ao alocar buffer de resposta\n");
        pbuf_free(p);
        return;
    }
    
    struct dhcp_msg *response = (struct dhcp_msg *)response_buf->payload;
    memset(response, 0, sizeof(struct dhcp_msg));
    
    // Preenche campos básicos
    response->op = 2; // BOOTREPLY
    response->htype = request->htype;
    response->hlen = request->hlen;
    response->xid = request->xid;
    response->magic_cookie = htonl(DHCP_MAGIC_COOKIE);
    
    // Copia MAC address do cliente
    memcpy(response->chaddr, request->chaddr, 16);
    
    // Atribui IP ao cliente (192.168.4.x)
    static uint32_t assigned_ip = 0; // Lembra o IP atribuído
    
    if (msg_type == 1) {
        // DISCOVER - oferece novo IP
        assigned_ip = 0xC0A80400 | next_ip; // 192.168.4.x
        next_ip++;
        if (next_ip > 254) next_ip = 2;
        printf("OFFER: Oferecendo IP 192.168.4.%d\n", assigned_ip & 0xFF);
    } else {
        // REQUEST - confirma o mesmo IP
        printf("ACK: Confirmando IP 192.168.4.%d\n", assigned_ip & 0xFF);
    }
    
    response->yiaddr = htonl(assigned_ip);
    
    // IP do servidor DHCP
    response->siaddr = htonl(0xC0A80401); // 192.168.4.1
    
    // Opções DHCP básicas
    uint8_t *opt = response->options;
    
    // Opção 53: DHCP Message Type (OFFER = 2, ACK = 5)
    *opt++ = 53; *opt++ = 1;
    *opt++ = (msg_type == 1) ? 2 : 5; // OFFER ou ACK
    
    // Opção 54: Server Identifier
    *opt++ = 54; *opt++ = 4;
    *opt++ = 192; *opt++ = 168; *opt++ = 4; *opt++ = 1;
    
    // Opção 51: Lease Time (1 hora = 3600s)
    *opt++ = 51; *opt++ = 4;
    uint32_t lease = htonl(3600);
    memcpy(opt, &lease, 4); opt += 4;
    
    // Opção 1: Subnet Mask
    *opt++ = 1; *opt++ = 4;
    *opt++ = 255; *opt++ = 255; *opt++ = 255; *opt++ = 0;
    
    // Opção 3: Router/Gateway
    *opt++ = 3; *opt++ = 4;
    *opt++ = 192; *opt++ = 168; *opt++ = 4; *opt++ = 1;
    
    // Opção 6: DNS Server
    *opt++ = 6; *opt++ = 4;
    *opt++ = 192; *opt++ = 168; *opt++ = 4; *opt++ = 1;
    
    // Opção 255: End
    *opt++ = 255;
    
    // Envia resposta broadcast
    ip_addr_t dest_ip;
    IP4_ADDR(&dest_ip, 255, 255, 255, 255);
    
    udp_sendto(pcb, response_buf, &dest_ip, DHCP_CLIENT_PORT);
    
    printf("Resposta DHCP enviada!\n\n");
    
    pbuf_free(response_buf);
    pbuf_free(p);
}

void dhcp_server_init(void) {
    struct udp_pcb *pcb = udp_new();
    
    if (pcb == NULL) {
        printf("ERRO: Falha ao criar UDP PCB para DHCP\n");
        return;
    }
    
    err_t err = udp_bind(pcb, IP_ADDR_ANY, DHCP_SERVER_PORT);
    if (err != ERR_OK) {
        printf("ERRO: Falha ao fazer bind DHCP na porta 67\n");
        udp_remove(pcb);
        return;
    }
    
    udp_recv(pcb, dhcp_recv, NULL);
    
    printf("✅ Servidor DHCP iniciado na porta 67\n");
    printf("   Distribuindo IPs: 192.168.4.2 - 192.168.4.254\n");
}
