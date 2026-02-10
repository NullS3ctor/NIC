#include "core/arp.h"
#include "core/ethernet.h"
#include "drivers/interface.h" 
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>


// ARP packet structure
struct arp_packet { //28 bytes
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t oper;
    uint8_t sha[6];
    uint32_t spa;
    uint8_t tha[6];
    uint32_t tpa;
} __attribute__((packed));

/****************** TX Functions ******************/

void arp_send_request(nic_driver_t *drv, nic_device_t *device, uint32_t target_ip) {
    uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    
    // Construir paquete ARP
    struct arp_packet arp_data;
    arp_data.htype = htons(1);
    arp_data.ptype = htons(0x0800);
    arp_data.hlen  = 6;
    arp_data.plen  = 4;
    arp_data.oper  = htons(ARP_REQUEST);
    memcpy(arp_data.sha, device->mac_address, 6);
    arp_data.spa = htonl(device->ip_address);
    memset(arp_data.tha, 0, 6);
    arp_data.tpa = htonl(target_ip);

    // Construir frame Ethernet con payload ARP
    uint8_t buf[ETH_HDR_LEN + sizeof(struct arp_packet)];
    int frame_len = eth_make_frame(buf, broadcast, device->mac_address, ETH_TYPE_ARP, &arp_data, sizeof(struct arp_packet));
    
    drv->send_packet(device, buf, frame_len);

    printf("[ARP] Request: Who has %d.%d.%d.%d?\n",
        (target_ip>>24)&0xFF,(target_ip>>16)&0xFF,
        (target_ip>>8)&0xFF,target_ip&0xFF);
}

void arp_send_reply(nic_driver_t *drv, nic_device_t *device, uint8_t *target_mac, uint32_t target_ip) {
    uint8_t buf[ETH_HDR_LEN + sizeof(struct arp_packet)];
    // Construir paquete ARP
    struct arp_packet arp_data;
    arp_data.htype = htons(1);
    arp_data.ptype = htons(0x0800);
    arp_data.hlen  = 6;
    arp_data.plen  = 4;
    arp_data.oper  = htons(ARP_REPLY);
    memcpy(arp_data.sha, device->mac_address, 6);
    arp_data.spa = htonl(device->ip_address);
    memcpy(arp_data.tha, target_mac, 6);
    arp_data.tpa = target_ip;

    // Construir frame Ethernet con payload ARP
    int frame_len = eth_make_frame(buf, target_mac, device->mac_address, ETH_TYPE_ARP, &arp_data, sizeof(struct arp_packet));
    
    drv->send_packet(device, buf, frame_len);

    printf("[ARP] Reply: %d.%d.%d.%d is at %02X:%02X:%02X:%02X:%02X:%02X\n",
        (target_ip>>24)&0xFF,(target_ip>>16)&0xFF,
        (target_ip>>8)&0xFF,target_ip&0xFF,
        target_mac[0],target_mac[1],target_mac[2],
        target_mac[3],target_mac[4],target_mac[5]);

    arp_table_add(target_ip, target_mac);
}


/****************** RX Function ******************/
/*
void arp_rx(nic_device_t *nic, nic_driver_t *drv,
            const uint8_t *frame, size_t len)
{
    struct arp_hdr *arp = (struct arp_hdr *)(frame + ETH_HDR_LEN);

    if (ntohs(arp->oper) != ARP_REQUEST)
        return;

    if (arp->target_ip != htonl(MY_IP))
        return;

    arp_send_reply(
        drv,
        nic,
        arp->sender_mac,
        ntohl(arp->sender_ip)
    );
}
*/
void arp_rx(uint8_t *buf, unsigned int len) {
    // 1. Usamos la constante ETH_HDR_LEN que ya tienes definida
    if (len < ETH_HDR_LEN + sizeof(struct arp_packet)) return;

    // 2. Para acceder al tipo, mapeamos la cabecera manualmente 
    // o usamos un puntero a los offsets de la trama
    uint16_t eth_type = (buf[12] << 8) | buf[13]; // El EtherType está en los bytes 12 y 13

    if (eth_type != 0x0806) return; // 0x0806 es el código para ARP (ETH_P_ARP)

    // 3. Apuntamos al paquete ARP que empieza después de la cabecera Ethernet
    struct arp_packet *arp = (void*)(buf + ETH_HDR_LEN);

    uint16_t oper = ntohs(arp->oper);

    // Solo procesamos Reply
    if (oper == 0x0002) { // 0x0002 es ARP_REPLY
        uint32_t sender_ip = ntohl(arp->spa);
        uint8_t *sender_mac = arp->sha;

        arp_table_add(sender_ip, sender_mac);

        printf("[ARP RX] Reply from %d.%d.%d.%d is at %02X:%02X:%02X:%02X:%02X:%02X\n",
            (sender_ip>>24)&0xFF, (sender_ip>>16)&0xFF,
            (sender_ip>>8)&0xFF, sender_ip&0xFF,
            sender_mac[0], sender_mac[1], sender_mac[2],
            sender_mac[3], sender_mac[4], sender_mac[5]);
    }
}


static arp_entry_t arp_table[ARP_TABLE_SIZE];

void arp_table_init(void) {
    memset(arp_table, 0, sizeof(arp_table)); // Vacía memoria
}

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

uint8_t *arp_table_lookup(uint32_t ip) {
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid && arp_table[i].ip == ip) {
            return arp_table[i].mac;
        }
    }
    return NULL;
}

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