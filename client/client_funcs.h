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
#define JSON_PORT 9999

double medir_rtt_promedio_con_tres_intentos(const char *server_ip, const char *etapa_nombre);
int parseo(int argc, char *argv[], const char **ip_servidor, bool *idle, bool *test_download, bool *test_upload, int *num_conn);
uint32_t generate_id();
double medir_rtt();
double download_test(const char *server_ip, char *src_ip, int num_conn);
void upload_test(const char *server_ip, uint32_t id, int num_conn);
void consultar_resultados(const char *ip, int udp_port, uint32_t id_measurement, struct BW_result *out_result, int num_conn);
void exportar_json(uint64_t bw_down, uint64_t bw_up, double rtt_idle, double rtt_down, double rtt_up, const char *src_ip, const char *dst_ip, int num_conn);

#endif // CLIENT_FUNCS_H