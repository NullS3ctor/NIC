#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include <stddef.h>

// Forward declaration para evitar dependencias circulares
struct nic_device;
typedef struct nic_device nic_device_t;

// Assumption: The IPv4 layer provides a type for IP addresses.
// If not, this needs to be defined (e.g., typedef uint32_t ipv4_addr_t;).
// For now, we'll use a placeholder.
typedef uint32_t ipv4_addr_t;


// TCP Header Flags
#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_URG 0x20

// TCP States
typedef enum {
    TCP_STATE_CLOSED,
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_SENT,
    TCP_STATE_SYN_RECEIVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT_1,
    TCP_STATE_FIN_WAIT_2,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_CLOSING,
    TCP_STATE_LAST_ACK,
    TCP_STATE_TIME_WAIT
} tcp_state_t;

// TCP Header Structure (packed to avoid padding)
#pragma pack(push, 1)
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t  data_offset; // 4 bits: header length, 4 bits: reserved
    uint8_t  flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_ptr;
} tcp_hdr_t;
#pragma pack(pop)

// TCP Connection Control Block (TCB)
// Stores the state of a single TCP connection
typedef struct {
    ipv4_addr_t local_ip;
    ipv4_addr_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;
    
    tcp_state_t state;

    uint32_t seq_num_next;      // Next sequence number to send
    uint32_t ack_num_expected;  // Next acknowledgment number we expect to receive

    // Buffers for sending and receiving data would go here
    // For a minimal implementation, we might handle data more directly

} tcb_t;


/*
 * ============================================================================
 *                              TCP API
 * ============================================================================
 */

/**
 * @brief Initializes the TCP layer.
 */
void tcp_init();

/**
 * @brief Handles an incoming TCP packet from the IPv4 layer.
 * 
 * @param nic A pointer to the nic_device that received the packet.
 * @param src_ip The source IP address of the packet.
 * @param packet A pointer to the start of the TCP packet (header + payload).
 * @param len The total length of the TCP packet.
 */
void tcp_input(nic_device_t* nic, ipv4_addr_t src_ip, void* packet, size_t len);

/**
 * @brief Sends data over a TCP connection.
 * 
 * @param nic A pointer to the nic_device for sending the packet.
 * @param tcb A pointer to the TCB for the connection.
 * @param data A pointer to the data to send.
 * @param len The length of the data to send.
 * @return int 0 on success, -1 on failure.
 */
int tcp_send(nic_device_t* nic, tcb_t* tcb, const void* data, size_t len);


/**
 * @brief Creates a new socket and puts it in the LISTEN state.
 *
 * @param port The local port to listen on.
 * @return A pointer to the new TCB, or NULL on failure.
 */
tcb_t* tcp_listen(uint16_t port);

/**
 * @brief Closes a TCP connection.
 *
 * @param tcb A pointer to the TCB of the connection to close.
 */
void tcp_close(tcb_t* tcb);


/**
 * @brief Callback function prototype for the application layer.
 *        The TCP layer will call this when a connection is established.
 *
 * @param tcb The TCB of the newly established connection.
 */
typedef void (*tcp_accept_callback_t)(tcb_t* tcb);


/**
 * @brief Callback function prototype for the application layer.
 *        The TCP layer will call this when data is received on a connection.
 *
 * @param tcb The TCB of the connection.
 * @param data Pointer to the received data.
 * @param len Length of the received data.
 */
typedef void (*tcp_data_callback_t)(tcb_t* tcb, void* data, size_t len);


/**
 * @brief Registers the callback functions for the application layer.
 * 
 * @param on_accept The function to call when a connection is accepted.
 * @param on_data The function to call when data is received.
 */
void tcp_register_callbacks(tcp_accept_callback_t on_accept, tcp_data_callback_t on_data);


#endif // TCP_H
