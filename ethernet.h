#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

// Tamaños básicos
#define ETH_MAC_LEN     6
#define ETH_HDR_LEN     14
#define ETH_MAX_DATA    1500

// Tipos de protocolo comunes
#define ETH_TYPE_IP     0x0800
#define ETH_TYPE_ARP    0x0806
#define ETH_TYPE_TEST   0x9000

// Estructura del frame Ethernet
typedef struct {
    uint8_t  dest_mac[ETH_MAC_LEN];
    uint8_t  src_mac[ETH_MAC_LEN];
    uint16_t type;
    uint8_t  data[ETH_MAX_DATA];
} eth_frame_t;

// Crear un frame Ethernet
int eth_make_frame(uint8_t *buffer, 
                   uint8_t *dest_mac,
                   uint8_t *src_mac,
                   uint16_t type,
                   void *data,
                   int data_len);

// Leer un frame Ethernet
int eth_read_frame(uint8_t *buffer, 
                   int buffer_len,
                   eth_frame_t *frame);

#endif
