#ifndef _ARP_H
#define _ARP_H

#include <stdint.h>
#include "interface.h"

// Tipos de ARP
#define ARP_REQUEST 1
#define ARP_REPLY   2
#define ETH_P_ARP   0x0806
#define ARP_TABLE_SIZE 8

// Ethernet frame header
struct ethernet_frame { //14 bytes
    uint8_t dst_mac[6]; // Destination MAC
    uint8_t src_mac[6]; // Source MAC
    uint16_t type; // Ethernet type (e.g., 0x0800 for IPv4, 0x0806 for ARP)
} __attribute__((packed));


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

struct arp_entry_t{
    uint32_t ip;
    uint8_t  mac[6];
    int      valid;
} __attribute__((packed));

// Funciones ARP
void arp_send_request(nic_driver_t *drv,nic_device_t *device,uint32_t target_ip);
void arp_send_reply(nic_driver_t *drv,nic_device_t *device,uint8_t *target_mac, uint32_t target_ip);
void arp_rx(uint8_t *buf, unsigned int len);

// Funciones Tabla ARP
void arp_table_init(void);
void arp_table_add(uint32_t ip, uint8_t *mac);
uint8_t *arp_table_lookup(uint32_t ip);
void arp_table_print(void);

#endif // ARP_H