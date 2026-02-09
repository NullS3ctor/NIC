# Registro de Cambios: Implementación de la Capa TCP

Este documento describe la nueva implementación de la capa de transporte TCP y cómo se integra con el resto del proyecto. Está destinado a que el resto del equipo pueda entender los cambios, el nuevo módulo y cómo utilizarlo.

## Resumen

Se ha añadido una implementación mínima del **Protocolo de Control de Transmisión (TCP)** en los ficheros `tcp.c` y `tcp.h`. Esta capa proporciona una comunicación fiable y orientada a la conexión, sentando las bases para el funcionamiento del servidor HTTP. Para integrar esta nueva capa, ha sido necesario realizar pequeñas modificaciones en la capa IPv4.

## 1. Nueva Implementación: Capa TCP (`tcp.c`, `tcp.h`)

### ¿Qué es?

La capa TCP se sitúa entre la capa de red (IPv4) y la capa de aplicación (nuestro servidor HTTP). Su principal objetivo es ofrecer un "canal" de comunicación fiable para que la aplicación no tenga que preocuparse por la pérdida o el desorden de los paquetes.

Funciona mediante **conexiones** que se establecen, mantienen y cierran, garantizando que todos los datos enviados por un extremo llegan correctamente al otro.

### Componentes Clave

- **`tcp.h`**: Define la interfaz pública y las estructuras de datos:
    - **`tcp_hdr_t`**: La estructura de la cabecera TCP que se añade a cada paquete. Contiene puertos, números de secuencia/ACK y flags (SYN, ACK).
    - **`tcb_t` (Transmission Control Block)**: Es una estructura que representa una única conexión. Almacena el estado actual de la conexión (escuchando, establecida, etc.), las IPs/puertos y los números de secuencia necesarios para mantener la comunicación.

- **`tcp.c`**: Contiene la lógica de la máquina de estados de TCP.
    - **`tcp_input()`**: Es el corazón del módulo. Procesa los paquetes TCP entrantes que le entrega la capa IPv4.
    - **`tcp_send()`**: Es la función que la capa de aplicación (HTTP) usará para enviar datos de vuelta al cliente.
    - **Manejo de Conexiones**: Implementa la lógica básica del "saludo de tres vías" (Three-Way Handshake) para aceptar nuevas conexiones de clientes.

## 2. Cómo Usar la Nueva Capa TCP (API)

Para que el servidor web funcione, la lógica principal de la aplicación (`main.c` o similar) deberá usar la siguiente API de TCP:

1.  **Inicialización:**
    -   `tcp_init()`: Llama a esta función una sola vez al arrancar el programa para inicializar la capa TCP.

2.  **Poner el servidor a la escucha:**
    -   `tcp_listen(uint16_t puerto)`: Llama a esta función para empezar a aceptar conexiones en un puerto determinado (ej. puerto 80 para HTTP).

3.  **Manejar eventos con Callbacks:** Este es el punto de unión clave con la capa de aplicación.
    -   `tcp_register_callbacks(on_accept, on_data)`: Debes pasarle dos funciones que tú crearás:
        -   `on_accept`: La capa TCP llamará a esta función cuando una nueva conexión se haya establecido por completo.
        -   `on_data`: La capa TCP llamará a esta función cada vez que lleguen datos (ej. una petición HTTP) de un cliente en una conexión ya establecida. Dentro de esta función es donde deberás llamar a `parse_request` y `handle_request` de la capa HTTP.

4.  **Enviar datos de vuelta:**
    -   `tcp_send(nic_device_t* nic, tcb_t* tcb, const void* datos, size_t longitud)`: Cuando tu función `on_data` haya procesado una petición y tenga una respuesta HTTP lista, debe usar `tcp_send` para enviarla de vuelta al cliente a través de la conexión (`tcb`) correspondiente.

## 3. Cambios en Ficheros Existentes

Para que la nueva capa TCP recibiera los paquetes que le corresponden, fue necesario modificar ligeramente la capa inferior (IPv4).

### `ipv4.c`

-   **Inclusión de `tcp.h`**: Al principio del fichero, ahora se incluye `tcp.h`.
-   **Modificación en `ipv4_receive()`**: Esta función es la que recibe todos los paquetes IP. Se ha modificado su lógica de multiplexación para que reconozca los paquetes TCP.
    -   **Antes**: Solo reconocía el protocolo ICMP (1) y el resto lo ignoraba.
    -   **Ahora**: Se ha añadido un `else if (hdr->protocol == 6)` que comprueba si el paquete es TCP. Si lo es, se lo entrega a nuestra nueva función `tcp_input()` para que lo procese.
    
    Este cambio es el **enlace fundamental** que conecta la capa de red con nuestra nueva capa de transporte.

### `tcp.h` y `tcp.c`

-   Para que la capa TCP pudiera enviar paquetes de vuelta (especialmente el SYN-ACK al iniciar una conexión), fue necesario que tuviera conocimiento del dispositivo de red (`nic_device_t`). Por ello, las funciones `tcp_input` y `tcp_send` ahora aceptan un puntero `nic_device_t*`. Esto permite que TCP le diga a IPv4 por qué "puerta" física debe sacar los paquetes.

## 4. Flujo de Datos (Ejemplo de Vida de una Petición)

Para resumir, el flujo de una petición HTTP ahora sería el siguiente:

#### Recepción de una Petición

1.  El `main.c` recibe un frame de la red.
2.  Lo pasa a `ipv4_receive()`.
3.  `ipv4_receive()` ve que el protocolo es 6 (TCP) y llama a `tcp_input()`.
4.  `tcp_input()` procesa el paquete y, si contiene datos de aplicación, llama al **callback `on_data`** que hemos registrado.
5.  Nuestra función `on_data` recibe los datos crudos, llama a la lógica de `http_server.c` para parsearlos y generar una respuesta.

#### Envío de una Respuesta

1.  Nuestra lógica de aplicación, tras generar la respuesta, llama a `tcp_send()`.
2.  `tcp_send()` construye el paquete TCP, añade las cabeceras y se lo pasa a `ipv4_send()`.
3.  `ipv4_send()` construye el paquete IP y lo envía a la red a través del driver de la NIC.
