#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "interface.h"
#include "ethernet.h"
#include <stdlib.h>
#include "hal.h"


void ethernet_input_callback(const void *data, unsigned int len) {
    eth_frame_t frame;
    
    int payload_len = eth_read_frame((uint8_t*)data, len, &frame);
    
    if (payload_len < 0) return; // Ignorar basura

    printf("\n[RX] Paquete recibido en el hilo del driver (%d bytes)\n", len);
    printf("   > Origen:  %02x:%02x:%02x:%02x:%02x:%02x\n",
           frame.src_mac[0], frame.src_mac[1], frame.src_mac[2],
           frame.src_mac[3], frame.src_mac[4], frame.src_mac[5]);
    printf("   > Destino: %02x:%02x:%02x:%02x:%02x:%02x\n",
           frame.dest_mac[0], frame.dest_mac[1], frame.dest_mac[2],
           frame.dest_mac[3], frame.dest_mac[4], frame.dest_mac[5]);
    printf("   > Tipo:    0x%04x\n", frame.type);

    switch (frame.type) {
        case ETH_TYPE_ARP:
            printf("   > Protocolo: ARP (0x0806)\n");
            break;
        case ETH_TYPE_IP:
            printf("   > Protocolo: IP (0x0800)\n");
            break;
        default:
            printf("   > Protocolo: Desconocido (0x%04x)\n", frame.type);
    }
    printf("-----------------------------------------------------------\n");
}



void ethernet_output(nic_driver_t *drv, nic_device_t *nic,  uint8_t *dest_mac, uint16_t tipo, 
                     uint8_t *datos, int len) {
    
    if(!drv || !nic){
        printf("[ETHERNET] Error: stack no inicializado\n");
        return;
    }
    
    uint8_t frame[2048];
    
    // Crear frame
    int frame_len = eth_make_frame(frame, dest_mac, nic->mac_address, tipo, datos, len);
    
    // VERIFICAR: Imprimir antes de enviar
    printf("Frame creado (%d bytes):\n", frame_len);
    for (int i = 0; i < frame_len; i++) {
        printf("%02x ", frame[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
    
    // Enviar
    if(frame_len > 0){
        printf("\n[MAIN] Ordenando envío al driver (%d bytes)...\n", frame_len);
        drv->send_packet(nic, frame, frame_len);
    } else {
        printf("[ETHERNET] Frame inválido - descartado\n");
    }
    
}

void on_tx_done(const void *data, unsigned int len) {
    printf("[TX] Hardware notifica: El paquete ha salido del buffer.\n");
}

void on_error(const void *data, unsigned int len) {
    printf("[ETHERNET] Error en transmisión/recepción\n");
}


int main() {
    // Inicializar NIC
    nic_device_t nic;
    nic_driver_t *drv = nic_get_driver();
    
    if (drv->init(&nic) != STATUS_OK) {
        printf("Error al inicializar NIC\n");
        return -1;
    }
    
    printf("NIC inicializado\n");
    printf("Tu MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           nic.mac_address[0], nic.mac_address[1], nic.mac_address[2],
           nic.mac_address[3], nic.mac_address[4], nic.mac_address[5]);
    
    // Registrar callbacks
    drv->ioctl(&nic, NIC_IOCTL_ADD_RX_CALLBACK, (void*)ethernet_input_callback);
    drv->ioctl(&nic, NIC_IOCTL_ADD_TX_CALLBACK, (void*)on_tx_done);
    drv->ioctl(&nic, NIC_IOCTL_ADD_ERROR_CALLBACK, (void*)on_error);
    

    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    char *mensaje = "Prueba hola";

   ethernet_output(
        drv,
        &nic,               // Puntero al dispositivo (para sacar mi MAC y el handle)
        broadcast,          // MAC Destino
        ETH_TYPE_ARP,      // Tipo de protocolo (0x9000 para pruebas)
        (uint8_t*)mensaje,  // Payload
        strlen(mensaje)     // Longitud del payload
    );

    printf("\nPresiona Enter para salir...\n");
    while(getchar() != '\n');

    //exit(0);
}
