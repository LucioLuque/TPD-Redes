CC = gcc
CFLAGS = -Wall -Wextra -O2

HANDLE_SRC = ../handle_result/handle_result.c
HANDLE_HDR = ../handle_result/handle_result.h

CLIENTE_SRC = ../client/client.c
CLIENTE_EXE = client

SERVER_SRC = ../server/server.c
SERVER_EXE = server

all: $(CLIENTE_EXE) $(SERVER_EXE)

# Cliente
$(CLIENTE_EXE): $(CLIENTE_SRC) $(HANDLE_SRC) $(HANDLE_HDR)
	$(CC) $(CFLAGS) -o $@ $(CLIENTE_SRC) $(HANDLE_SRC)

# Servidor
$(SERVER_EXE): $(SERVER_SRC) $(HANDLE_SRC) $(HANDLE_HDR)
	$(CC) $(CFLAGS) -o $@ $(SERVER_SRC) $(HANDLE_SRC) -lpthread

#clean
clean:
	rm -f $(CLIENTE_EXE) $(SERVER_EXE)

.PHONY: all clean
