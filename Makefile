# Makefile for NIC Project

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Isrc/include
LDFLAGS = -lpthread
DEBUG = -g -O0
RELEASE = -O2

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = .

# Source files
CORE_SRCS = $(SRC_DIR)/core/arp.c $(SRC_DIR)/core/icmp.c $(SRC_DIR)/core/ipv4.c $(SRC_DIR)/core/ethernet.c
DRIVERS_SRCS = $(SRC_DIR)/drivers/hal.c $(SRC_DIR)/drivers/interface.c
# Excluimos server.c de NETWORK_SRCS porque tiene su propio main()
NETWORK_SRCS = $(SRC_DIR)/network/tcp.c $(SRC_DIR)/network/http_server.c

# El ejecutable principal usará main.c y todas las librerías anteriores
MAIN_SRCS = main.c $(CORE_SRCS) $(DRIVERS_SRCS) $(NETWORK_SRCS)

# Object files
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(MAIN_SRCS))

# Output executable
TARGET = $(BIN_DIR)/nicnet

# Default target
.PHONY: all
all: CFLAGS += $(RELEASE)
all: $(TARGET)

# Debug target
.PHONY: debug
debug: CFLAGS += $(DEBUG)
debug: $(TARGET)

# Build executable
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Compilación exitosa: $@"

# Build object files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "✓ Compilado: $<"

# --- OPCIONAL: Regla para compilar el servidor independiente ---
.PHONY: server
server: CFLAGS += $(RELEASE)
server: $(BIN_DIR)/network_server

$(BIN_DIR)/network_server: $(patsubst %.c,$(BUILD_DIR)/%.o, $(SRC_DIR)/network/server.c $(CORE_SRCS) $(DRIVERS_SRCS) $(NETWORK_SRCS))
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Servidor independiente compilado: $@"

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	rm -f $(BIN_DIR)/network_server
	@echo "✓ Archivos limpios"

# Clean and rebuild
.PHONY: rebuild
rebuild: clean all

# Run the executable (requires sudo for network access)
.PHONY: run
run: $(TARGET)
	sudo $(TARGET)

# Run in debug mode
.PHONY: run-debug
run-debug: debug run

# Show help
.PHONY: help
help:
	@echo "Comandos disponibles:"
	@echo "  make            - Compila el proyecto principal (nicnet)"
	@echo "  make server     - Compila el servidor independiente (network_server)"
	@echo "  make debug      - Compila con símbolos de debug"
	@echo "  make rebuild    - Limpia y recompila"
	@echo "  make run        - Compila y ejecuta (requiere sudo)"
	@echo "  make clean      - Limpia archivos compilados"