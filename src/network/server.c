#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Incluimos tu cabecera para poder llamar a tus funciones
#include "network/http_server.h"

#define PORT 8080
#define BUFFER_SIZE 8192  // 8KB de buffer para peticiones grandes

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Buffers
    char buffer[BUFFER_SIZE] = {0};
    char response_buffer[BUFFER_SIZE] = {0}; // Buffer para la respuesta
    
    // Estructuras de tu lógica HTTP
    HttpRequest request;
    size_t response_len;

    // 1. Crear el socket (IPv4, TCP)
    // AF_INET = IPv4, SOCK_STREAM = TCP
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Fallo al crear el socket");
        exit(EXIT_FAILURE);
    }

    // 2. Configurar opciones del socket
    // ESTO ES IMPORTANTE: Permite reutilizar el puerto inmediatamente si cierras y abres el servidor.
    // Sin esto, tendrías que esperar minutos si el servidor se cuelga.
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // 3. Definir la dirección y puerto
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Escuchar en cualquier interfaz de red
    address.sin_port = htons(PORT);       // Puerto 8080 (convertido a network byte order)

    // 4. Bind (Vincular el socket al puerto)
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Fallo en bind (¿Puerto ocupado?)");
        exit(EXIT_FAILURE);
    }

    // 5. Listen (Ponerse a escuchar)
    // El '10' es el tamaño de la cola de espera de conexiones
    if (listen(server_fd, 10) < 0) {
        perror("Fallo en listen");
        exit(EXIT_FAILURE);
    }

    printf("\n=== SERVIDOR HTTP INICIADO EN EL PUERTO %d ===\n", PORT);
    printf("Esperando conexiones...\n\n");

    // 6. Bucle infinito principal
    while(1) {
        // A. Aceptar nueva conexión (Esto bloquea el programa hasta que alguien se conecte)
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Error en accept");
            continue;
        }

        // B. Limpiar buffers antes de usar (Seguridad)
        memset(buffer, 0, BUFFER_SIZE);
        memset(response_buffer, 0, BUFFER_SIZE);

        // C. Leer datos del cliente
        int valread = read(new_socket, buffer, BUFFER_SIZE);
        if (valread > 0) {
            
            // --- AQUÍ CONECTAMOS CON TU CÓDIGO ---
            
            // 1. Parsear la petición
            parse_request(buffer, &request);
            
            printf("--> Petición: %s %s (Body: %d bytes)\n", 
                   request.method, request.path, request.content_length);

            // 2. Generar la respuesta usando tu lógica
            handle_request(&request, response_buffer, &response_len);

            // 3. Enviar la respuesta de vuelta por el socket
            send(new_socket, response_buffer, response_len, 0);
            
            printf("<-- Respuesta enviada (%zu bytes): %s\n", response_len, request.path);
            printf("----------------------------------------\n");
        }

        // D. Cerrar la conexión con este cliente
        close(new_socket);
    }

    return 0;
}