# Nombre del ejecutable
TARGET = client

SRC = client.c

#Compilador
CC = gcc

# Flags
CFLAGS = -Wall -g

# Regla por defecto
all: $(TARGET)

# Se construye el ejecutable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# Clean
clean:
	rm -f $(TARGET)
