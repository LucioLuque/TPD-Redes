#ifndef SERVER_FUNCS_H
#define SERVER_FUNCS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <inttypes.h>
#include <sys/time.h>
#include <signal.h>
#include <stdbool.h>
#include "handle_result.h"
#include "config.h"

#define MAX_CLIENTS 100

struct ResultadoEntry {
    struct BW_result result;
    struct ResultadoEntry *next;
};

struct thread_arg_t {
    int client_sock;
    struct sockaddr_in client_addr;
};

// struct ResultadoEntry *resultados = NULL;
extern struct ResultadoEntry *resultados;



void *handle_download_conn(void *arg);
void *handle_upload_conn(void *arg);
void *udp_server_thread(void *arg);
struct BW_result *obtener_o_crear_resultado(uint32_t id_measurement);

#endif // SERVER_FUNCS_H
