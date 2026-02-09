#include "ipv4.h"
#include "interface.h"
#include "icmp.h"
#include "arp.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

/**
 * Calcula el checksum de Internet (RFC 1071) para la cabecera IP.
 * Se utiliza para verificar la integridad de la cabecera en la recepción.
 */
uint16_t ipv4_checksum(void *vdata, size_t length) {
    uint32_t sum = 0;
    uint16_t *data = vdata;
    
    while (length > 1) {
        sum += *data++;
        length -= 2;
    }
    if (length > 0) {
        sum += *(uint8_t *)data;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum;
}

void ipv4_send(nic_device_t *nic, uint32_t dst_ip, uint8_t protocol, const void *data, uint16_t data_len) {
    nic_driver_t *drv = nic_get_driver();

    // Convertimos a orden de host para la tabla ARP y para debug
    uint32_t dst_ip_h = ntohl(dst_ip);

    // 1. Intentar obtener la MAC de destino mediante la tabla ARP
    uint8_t *dst_mac = arp_table_lookup(dst_ip_h);

    if (dst_mac == NULL) {
        struct in_addr a; a.s_addr = dst_ip;
        printf("[IPv4] MAC desconocida para %s, enviando ARP Request...\n", inet_ntoa(a));
        arp_send_request(drv, nic, dst_ip_h);
        return;
    }

    // 2. Construimos el frame completo (Ethernet + IP)
    uint16_t ip_len = sizeof(struct ipv4_header) + data_len;
    uint16_t total_len = sizeof(struct ethernet_frame) + ip_len;
    uint8_t buf[total_len];

    // Punteros a las distintas partes del buffer
    struct ethernet_frame *eth = (void*)buf;
    struct ipv4_header *ip = (void*)(buf + sizeof(*eth));

    // 3. Rellenar Ethernet
    memcpy(eth->dst_mac, dst_mac, 6);
    memcpy(eth->src_mac, nic->mac_address, 6);
    eth->type = htons(ETH_P_IP);

    // 4. Rellenar Header IPv4
    ip->version_ihl = (4 << 4) | (sizeof(struct ipv4_header) / 4);
    ip->type_of_service = 0;
    ip->total_length = htons(ip_len);
    ip->identification = htons(0);
    ip->flags_fragment_offset = htons(0);
    ip->time_to_live = 64;
    ip->protocol = protocol;
    ip->header_checksum = 0;
    ip->source_address = nic->ip_address; // ya en network order
    ip->destination_address = dst_ip;    // ya en network order
    ip->header_checksum = ipv4_checksum(ip, sizeof(struct ipv4_header));

    // 5. Copiar Payload (ICMP, TCP, etc.)
    if (data && data_len > 0) {
        memcpy(buf + sizeof(*eth) + sizeof(struct ipv4_header), data, data_len);
    }

    // 6. Enviar al driver
    drv->send_packet(nic, buf, total_len);
}
/**
 * Procesa un paquete IPv4 entrante recibido desde la capa Ethernet.
 */
void ipv4_receive(nic_device_t *nic, const void *packet, unsigned int len) {
    struct ipv4_header *hdr = (struct ipv4_header *)packet;

    // 1. Validar integridad de la cabecera
    if (ipv4_checksum(hdr, sizeof(struct ipv4_header)) != 0) {
        return; 
    }

    // 2. Filtrar por dirección IP (Unicast a nuestra IP o Broadcast limitado)
    if (hdr->destination_address != nic->ip_address && hdr->destination_address != 0xFFFFFFFF) {
        return; 
    }

    // 3. Calcular ubicación y tamaño del payload IP
    uint16_t ip_hdr_len = (hdr->version_ihl & 0x0F) * 4;
    unsigned char *payload = (unsigned char *)packet + ip_hdr_len;
    uint16_t payload_len = ntohs(hdr->total_length) - ip_hdr_len;

    // 4. Multiplexación: Derivar según el protocolo de la capa de transporte
    if (hdr->protocol == 1) { 
        // Protocolo ICMP
        icmp_receive(nic, hdr->source_address, payload, payload_len);
    } else {
        // Otros protocolos (TCP/UDP/Experimental)
        struct in_addr src_addr;
        src_addr.s_addr = hdr->source_address;

        printf("[IP] Paquete recibido de: %s\n", inet_ntoa(src_addr));
        printf("   | Protocolo: %u | Longitud Datos: %u\n", hdr->protocol, payload_len);
        
        if (payload_len > 0) {
            printf("   | Contenido (Hex): ");
            for(int i = 0; i < (payload_len > 16 ? 16 : payload_len); i++) {
                printf("%02x ", payload[i]);
            }
            printf("\n");
        }
    }
}