#include <string.h>
#include <arpa/inet.h>
#include "ethernet.h"

// Crear un frame Ethernet
int eth_make_frame(uint8_t *buffer,
                   uint8_t *dest_mac,
                   uint8_t *src_mac,
                   uint16_t type,
                   void *data,
                   int data_len) {
    
    if (!buffer || !dest_mac || !src_mac || data_len > ETH_MAX_DATA)
        return -1;
    
    int pos = 0;
    
    // Copiar MAC destino
    memcpy(buffer + pos, dest_mac, ETH_MAC_LEN);
    pos += ETH_MAC_LEN;
    
    // Copiar MAC origen
    memcpy(buffer + pos, src_mac, ETH_MAC_LEN);
    pos += ETH_MAC_LEN;
    
    // Copiar tipo (network byte order)
    uint16_t type_be = htons(type);
    memcpy(buffer + pos, &type_be, 2);
    pos += 2;
    
    // Copiar datos
    if (data && data_len > 0) {
        memcpy(buffer + pos, data, data_len);
        pos += data_len;
    }
    
    return pos; // Retorna tamaño total
}

// Leer un frame Ethernet
int eth_read_frame(uint8_t *buffer,
                   int buffer_len,
                   eth_frame_t *frame) {
    
    if (!buffer || !frame || buffer_len < ETH_HDR_LEN)
        return -1;
    
    int pos = 0;
    
    // Leer MAC destino
    memcpy(frame->dest_mac, buffer + pos, ETH_MAC_LEN);
    pos += ETH_MAC_LEN;
    
    // Leer MAC origen
    memcpy(frame->src_mac, buffer + pos, ETH_MAC_LEN);
    pos += ETH_MAC_LEN;
    
    // Leer tipo (convertir de network byte order)
    uint16_t type_be;
    memcpy(&type_be, buffer + pos, 2);
    frame->type = ntohs(type_be);
    pos += 2;
    
    // Leer datos
    int data_len = buffer_len - ETH_HDR_LEN;
    if (data_len > 0 && data_len <= ETH_MAX_DATA) {
        memcpy(frame->data, buffer + pos, data_len);
        return data_len; // Retorna tamaño de datos
    }
    
    return 0;
}
