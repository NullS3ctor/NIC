#ifndef _ARP_H
#define _ARP_H

#include <stdint.h>
#include "drivers/interface.h"

// Tipos de ARP
#define ARP_REQUEST 1
#define ARP_REPLY   2
#define ETH_P_ARP   0x0806
#define ARP_TABLE_SIZE 8

// Funciones públicas
void arp_send_request(nic_driver_t *drv,nic_device_t *device,uint32_t target_ip);
void arp_send_reply(nic_driver_t *drv,nic_device_t *device,uint8_t *target_mac, uint32_t target_ip);
void arp_rx(uint8_t *buf, unsigned int len);

typedef struct {
    uint32_t ip;
    uint8_t  mac[6];
    int      valid;
} arp_entry_t;

void arp_table_init(void);
void arp_table_add(uint32_t ip, uint8_t *mac);
uint8_t *arp_table_lookup(uint32_t ip);
void arp_table_print(void);


#endif // ARP_H