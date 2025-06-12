#ifndef CLIENT_FUNCS_H
#define CLIENT_FUNCS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <stdbool.h>
#include <pthread.h>

#include "handle_result.h"
#include "config.h"

#define SERVER_IP "127.0.0.1"

int parse_arguments(int argc, char *argv[], const char **ip_servidor, int *num_conn, 
    bool *idle, bool *test_download, bool *test_upload, bool *json,
    const char **ip_hostremoto, int *json_port);
double calculate_rtt();
double rtt_test(const char *server_ip, 
        const char *etapa_nombre);
double download_test(const char *server_ip, char *src_ip, int num_conn, 
        double *rtt_download);
uint32_t generate_id();
void upload_test(const char *server_ip, uint32_t id, int num_conn, double *rtt_upload);
void query_results_from_server(const char *ip, int udp_port, uint32_t id_measurement,
        struct BW_result *out_result, int num_conn);
void export_json(uint64_t bw_down, uint64_t bw_up, double rtt_idle,
        double rtt_down, double rtt_up, const char *src_ip, const char *dst_ip,
        int num_conn, const char *ip_hostremoto, int json_port);

#endif // CLIENT_FUNCS_H