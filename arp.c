#include "arp.h"
#include "interface.h" 
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>

static struct arp_entry_t arp_table[ARP_TABLE_SIZE];


/****************** TX Functions ******************/
// Envía un ARP Request para resolver la MAC de una IP destino
void arp_send_request(nic_driver_t *drv, nic_device_t *device, uint32_t target_ip) {
    uint8_t buf[42];
    struct ethernet_frame *eth = (void*)buf;
    struct arp_packet *arp = (void*)(buf + sizeof(*eth));

    uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    
    // Construimos la cabecera Ethernet
    memcpy(eth->dst_mac, broadcast, 6);
    memcpy(eth->src_mac, device->mac_address, 6);
    eth->type = htons(ETH_P_ARP);
    
    // Construimos el paquete ARP
    arp->htype = htons(1);
    arp->ptype = htons(0x0800);
    arp->hlen  = 6;
    arp->plen  = 4;
    arp->oper  = htons(ARP_REQUEST);
    memcpy(arp->sha, device->mac_address, 6);
    arp->spa = device->ip_address;
    memset(arp->tha, 0, 6);
    arp->tpa = htonl(target_ip);

    // Enviamos el paquete
    drv->send_packet(device, buf, sizeof(buf));

    printf("[ARP] Request: Who has %d.%d.%d.%d?\n",
        (target_ip>>24)&0xFF,(target_ip>>16)&0xFF,
        (target_ip>>8)&0xFF,target_ip&0xFF);
}

// Envía un ARP Reply en respuesta a un Request recibido
void arp_send_reply(nic_driver_t *drv, nic_device_t *device, uint8_t *target_mac, uint32_t target_ip) {
    uint8_t buf[42];
    struct ethernet_frame *eth = (void*)buf;
    struct arp_packet *arp = (void*)(buf + sizeof(*eth));

    // Construimos la cabecera Ethernet
    memcpy(eth->dst_mac, target_mac, 6);
    memcpy(eth->src_mac, device->mac_address, 6);
    eth->type = htons(ETH_P_ARP);

    // Construimos el paquete ARP
    arp->htype = htons(1);
    arp->ptype = htons(0x0800);
    arp->hlen  = 6;
    arp->plen  = 4;
    arp->oper  = htons(ARP_REPLY);
    memcpy(arp->sha, device->mac_address, 6);
    arp->spa = device->ip_address;
    memcpy(arp->tha, target_mac, 6);
    arp->tpa = htonl(target_ip);
    // Enviamos el paquete
    drv->send_packet(device, buf, sizeof(buf));

    printf("[ARP] Reply: %d.%d.%d.%d is at %02X:%02X:%02X:%02X:%02X:%02X\n",
        (target_ip>>24)&0xFF,(target_ip>>16)&0xFF,
        (target_ip>>8)&0xFF,target_ip&0xFF,
        target_mac[0],target_mac[1],target_mac[2],
        target_mac[3],target_mac[4],target_mac[5]);

    arp_table_add(target_ip, target_mac);
}

/****************** RX Function ******************/
// Procesa un paquete ARP recibido. Solo se llama desde main.c cuando llega un paquete con ethertype 0x0806
void arp_rx(uint8_t *buf, unsigned int len) {
    if (len < sizeof(struct ethernet_frame) + sizeof(struct arp_packet)) return;

    struct ethernet_frame *eth = (void*)buf; // apunta a la cabecera Ethernet al inicio del buffer.
    struct arp_packet *arp = (void*)(buf + sizeof(*eth)); // apunta justo después de la cabecera Ethernet, donde empieza el paquete ARP.

    if (ntohs(eth->type) != ETH_P_ARP) return;

    uint16_t oper = ntohs(arp->oper);

    // Solo procesamos Reply
    if (oper == ARP_REPLY) {
        uint32_t sender_ip = ntohl(arp->spa);
        uint8_t *sender_mac = arp->sha;

        arp_table_add(sender_ip, sender_mac);

        printf("[ARP RX] Reply from %d.%d.%d.%d is at %02X:%02X:%02X:%02X:%02X:%02X\n",
            (sender_ip>>24)&0xFF,(sender_ip>>16)&0xFF,
            (sender_ip>>8)&0xFF,sender_ip&0xFF,
            sender_mac[0],sender_mac[1],sender_mac[2],
            sender_mac[3],sender_mac[4],sender_mac[5]);
    }
}

/****************** ARP Table ******************/

void arp_table_init(void) {
    memset(arp_table, 0, sizeof(arp_table)); // Vacía memoria
}

// Añade o actualiza una entrada en la tabla ARP
void arp_table_add(uint32_t ip, uint8_t *mac) {
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (!arp_table[i].valid) {
            arp_table[i].ip = ip;
            memcpy(arp_table[i].mac, mac, 6);
            arp_table[i].valid = 1;
            return;
        }
    }
    // Si la tabla está llena, reemplaza la primera entrada (simple)
    arp_table[0].ip = ip;
    memcpy(arp_table[0].mac, mac, 6);
    arp_table[0].valid = 1;
}

// Busca la MAC correspondiente a una IP en la tabla ARP. Devuelve NULL si no se encuentra.
uint8_t *arp_table_lookup(uint32_t ip) {
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid && arp_table[i].ip == ip) {
            return arp_table[i].mac;
        }
    }
    return NULL;
}

// Imprime el contenido de la tabla ARP
void arp_table_print(void) {
    printf("\nARP table:\n");
    printf("IP address        MAC address\n");
    printf("----------------  -----------------\n");
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid) {
            uint32_t ip = arp_table[i].ip;
            uint8_t *m = arp_table[i].mac;
            printf("%d.%d.%d.%d     %02X:%02X:%02X:%02X:%02X:%02X\n",
                (ip>>24)&0xFF, (ip>>16)&0xFF,
                (ip>>8)&0xFF, ip&0xFF,
                m[0],m[1],m[2],m[3],m[4],m[5]);
        }
    }
}