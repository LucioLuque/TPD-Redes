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

struct ResultEntry {
    char client_ip[INET_ADDRSTRLEN];
    struct BW_result result;
    struct ResultEntry *next;
};

struct BW_result {
  uint32_t id_measurement;
  uint64_t conn_bytes[NUM_CONN_MAX];
  double conn_duration[NUM_CONN_MAX];
};


struct thread_arg_t {
    int client_sock;
    struct sockaddr_in client_addr;
};

extern struct ResultEntry *results;

void *handle_download_conn(void *arg);
void *handle_upload_conn(void *arg);
void *udp_server_thread(void *arg);
struct BW_result *get_or_create_result(uint32_t id_measurement, 
                                       const char *client_ip);

#endif
