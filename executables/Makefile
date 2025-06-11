# Makefile (to be placed in executables/)

CC      := gcc
CFLAGS  := -Wall -Wextra -I../common

# Source sets
SERVER_SRCS := ../common/handle_result.c \
               ../server/server_funcs.c \
               ../server/server.c

CLIENT_SRCS := ../common/handle_result.c \
               ../client/client_funcs.c \
               ../client/client.c

.PHONY: all clean

all: server client

# Build the server executable (no intermediate .o files left)
server:
	$(CC) $(CFLAGS) $(SERVER_SRCS) -o server

# Build the client executable
client:
	$(CC) $(CFLAGS) $(CLIENT_SRCS) -o client

clean:
	rm -f server client
