#include "network/tcp.h"
#include "core/ipv4.h" // <--- MODIFICACION: Incluir para llamar a ipv4_send
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// For demonstration purposes, we'll use a static array for TCBs.
// A real implementation would use dynamic allocation.
#define MAX_TCP_CONNECTIONS 10
static tcb_t connection_pool[MAX_TCP_CONNECTIONS];

// Application layer callbacks
static tcp_accept_callback_t app_on_accept = NULL;
static tcp_data_callback_t app_on_data = NULL;

// Forward declaration for internal helper
static void send_tcp_packet(nic_device_t* nic, tcb_t* tcb, uint8_t flags, const void* data, size_t len);


/*
 * ============================================================================
 *                          Initialization and Setup
 * ============================================================================
 */

void tcp_init() {
    printf("Initializing TCP layer...\n");
    memset(connection_pool, 0, sizeof(connection_pool));
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        connection_pool[i].state = TCP_STATE_CLOSED;
    }
    printf("TCP layer initialized.\n");
}

void tcp_register_callbacks(tcp_accept_callback_t on_accept, tcp_data_callback_t on_data) {
    app_on_accept = on_accept;
    app_on_data = on_data;
}


/*
 * ============================================================================
 *                             Core Packet Processing
 * ============================================================================
 */

// Finds a TCB that matches the incoming packet's properties
tcb_t* find_tcb(ipv4_addr_t remote_ip, uint16_t remote_port, uint16_t local_port) {
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (connection_pool[i].state != TCP_STATE_CLOSED &&
            connection_pool[i].remote_ip == remote_ip &&
            connection_pool[i].remote_port == remote_port &&
            connection_pool[i].local_port == local_port) {
            return &connection_pool[i];
        }
    }
    return NULL;
}

// Finds a TCB that is in the LISTEN state on a specific port
tcb_t* find_listening_tcb(uint16_t local_port) {
     for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (connection_pool[i].state == TCP_STATE_LISTEN &&
            connection_pool[i].local_port == local_port) {
            return &connection_pool[i];
        }
    }
    return NULL;
}


void tcp_input(nic_device_t* nic, ipv4_addr_t src_ip, void* packet, size_t len) {
    if (len < sizeof(tcp_hdr_t)) {
        printf("TCP packet too short.\n");
        return;
    }

    tcp_hdr_t* hdr = (tcp_hdr_t*)packet;
    
    // In a real implementation, checksum would be verified here.

    // Find the connection this packet belongs to
    tcb_t* tcb = find_tcb(src_ip, hdr->src_port, hdr->dst_port);
    if (!tcb) {
        // If no existing connection, check for a listening socket (for new connections)
        tcb = find_listening_tcb(hdr->dst_port);
    }

    if (!tcb) {
        // No matching connection or listener, maybe send RST (not implemented)
        printf("TCP packet for unknown connection.\n");
        return;
    }

    // --- State Machine ---
    switch (tcb->state) {
        case TCP_STATE_LISTEN:
            // This is where a server starts a new connection.
            if (hdr->flags & TCP_FLAG_SYN) {
                printf("Received SYN on listening port %u\n", ntohs(tcb->local_port));
                
                // This TCB should spawn a new TCB for the actual connection.
                // For simplicity here, we'll transition this one. A real server
                // would keep the listener and create a new TCB.
                tcb->state = TCP_STATE_SYN_RECEIVED;
                tcb->remote_ip = src_ip;
                tcb->remote_port = hdr->src_port;
                tcb->ack_num_expected = ntohl(hdr->seq_num) + 1; // We need to ACK their SYN
                tcb->seq_num_next = 0; // Our SYN will have its own sequence number

                // Send SYN-ACK
                printf("Sending SYN-ACK...\n");
                send_tcp_packet(nic, tcb, TCP_FLAG_SYN | TCP_FLAG_ACK, NULL, 0);

            }
            break;

        case TCP_STATE_SYN_RECEIVED:
            // We sent a SYN-ACK, now we expect an ACK back.
            if ((hdr->flags & TCP_FLAG_ACK) && (ntohl(hdr->ack_num) == tcb->seq_num_next + 1)) {
                printf("Received ACK, connection established!\n");
                tcb->state = TCP_STATE_ESTABLISHED;
                tcb->seq_num_next++; // Our SYN is now acknowledged

                // Inform the application layer
                if (app_on_accept) {
                    app_on_accept(tcb);
                }
            }
            break;

        case TCP_STATE_ESTABLISHED:
            // Handle incoming data, FIN, etc.
            // For now, just print a message.
            printf("Received packet on established connection.\n");
            
            // If there's data, pass it up to the application
            size_t header_len = (hdr->data_offset >> 4) * 4;
            size_t payload_len = len - header_len;

            if (payload_len > 0 && app_on_data) {
                app_on_data(tcb, (uint8_t*)packet + header_len, payload_len);
            }
            
            break;
        
        // Other states (FIN_WAIT, CLOSE_WAIT, etc.) would be handled here.
        default:
            printf("TCP packet received in unhandled state.\n");
            break;
    }
}


/*
 * ============================================================================
 *                                 TCP API
 * ============================================================================
 */

tcb_t* tcp_listen(uint16_t port) {
    for (int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        if (connection_pool[i].state == TCP_STATE_CLOSED) {
            tcb_t* tcb = &connection_pool[i];
            memset(tcb, 0, sizeof(tcb_t));
            tcb->state = TCP_STATE_LISTEN;
            tcb->local_port = htons(port);
            printf("TCP listening on port %u\n", port);
            return tcb;
        }
    }
    printf("Error: No available TCBs for listening.\n");
    return NULL;
}

void tcp_close(tcb_t* tcb) {
    if (!tcb) return;

    // A real implementation would go through the FIN handshake.
    // Here we'll just close it abruptly.
    printf("Closing TCP connection.\n");
    tcb->state = TCP_STATE_CLOSED;
}


/*
 * ============================================================================
 *                       Internal Packet Sending Helper
 * ============================================================================
 */

// This function now takes the nic device to pass down to the ipv4_send function
static void send_tcp_packet(nic_device_t* nic, tcb_t* tcb, uint8_t flags, const void* data, size_t len) {
    size_t tcp_header_size = sizeof(tcp_hdr_t);
    size_t packet_size = tcp_header_size + len;
    uint8_t* packet = malloc(packet_size);

    if (!packet) {
        printf("Failed to allocate memory for TCP packet.\n");
        return;
    }

    tcp_hdr_t* hdr = (tcp_hdr_t*)packet;
    memset(hdr, 0, tcp_header_size);

    hdr->src_port = tcb->local_port;
    hdr->dst_port = tcb->remote_port;
    hdr->seq_num = htonl(tcb->seq_num_next);
    hdr->ack_num = htonl(tcb->ack_num_expected);
    hdr->data_offset = (tcp_header_size / 4) << 4;
    hdr->flags = flags;
    hdr->window_size = htons(8192); // Hardcoded window size
    
    if (data && len > 0) {
        memcpy(packet + tcp_header_size, data, len);
    }
    
    // Checksum calculation would go here.

    printf("Attempting to send TCP packet (flags: 0x%02X) via IPv4...\n", flags);
    
    /****************************************************************************
     * INICIO DE LA MODIFICACION: Reemplazo del STUB por la llamada a ipv4_send
     ****************************************************************************/
    // El protocolo 6 es TCP
    ipv4_send(nic, tcb->remote_ip, 6, packet, packet_size);
    /****************************************************************************
     * FIN DE LA MODIFICACION
     ****************************************************************************/

    free(packet);
}

int tcp_send(nic_device_t* nic, tcb_t* tcb, const void* data, size_t len) {
    if (!tcb || tcb->state != TCP_STATE_ESTABLISHED) {
        printf("Cannot send data on non-established connection.\n");
        return -1;
    }

    send_tcp_packet(nic, tcb, TCP_FLAG_ACK | TCP_FLAG_PSH, data, len);
    
    // A real implementation would wait for an ACK and handle retransmissions.
    tcb->seq_num_next += len;

    return 0;
}
