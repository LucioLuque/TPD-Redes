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

#include "../handle_result/handle_result.h"

#define SERVER_IP "127.0.0.1"
#define PORT_DOWNLOAD 20251
#define PORT_UPLOAD 20252
#define BUFFER_SIZE 1024
#define NUM_CONN 10
#define T 20

uint32_t generate_id();
double medir_rtt();
uint64_t download_test(char *src_ip, char *dst_ip);
void upload_test(uint32_t id);
void consultar_resultados(uint32_t id, struct BW_result* resultado);
void exportar_json(uint64_t bw_down, struct BW_result resultado, double rtt_idle, double rtt_down, double rtt_up, const char *src_ip, const char *dst_ip);

int main() {
    // Paso 1: medir latencia antes de todo (fase idle)
    double rtt_idle = medir_rtt();
    if (rtt_idle < 0) exit(1);

    // Paso 2: download test
    double rtt_down = medir_rtt();
    char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
    uint64_t total_bytes = download_test(src_ip, dst_ip);
    double bw_download_bps = (double)total_bytes * 8 / T;

    // Paso 3: upload test
    double rtt_up = medir_rtt();
    uint32_t id = generate_id();
    upload_test(id);

    // Paso 4: consultar resultados
    struct BW_result resultado;
    consultar_resultados(id, &resultado);

    // Paso 5: exportar en JSON por UDP
    exportar_json(bw_download_bps, resultado, rtt_idle, rtt_down, rtt_up, src_ip, dst_ip);
    return 0;
}


    

uint32_t generate_id() {
    srandom(time(NULL));
    uint32_t id = (uint32_t)random();
    if ((id >> 24) == 0xff) id &= 0x7fffffff;
    return id;
}

double medir_rtt() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server;
    socklen_t len = sizeof(server);

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT_DOWNLOAD);
    inet_pton(AF_INET, SERVER_IP, &server.sin_addr);

    uint8_t probe[4] = {0xff, 0x00, 0x00, 0x00};
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    sendto(sock, probe, 4, 0, (struct sockaddr*)&server, len);

    struct timeval timeout = {10, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int r = recvfrom(sock, probe, 4, 0, (struct sockaddr*)&server, &len);
    clock_gettime(CLOCK_MONOTONIC, &end);

    close(sock);

    if (r != 4 || probe[0] != 0xff) {
        fprintf(stderr, "[X] Error al recibir eco de RTT\n");
        return -1;
    }

    double rtt = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1e9;
    printf("[✓] RTT: %.3f segundos\n", rtt);
    sleep(1); // separa mediciones
    return rtt;
}

uint64_t download_test(char *src_ip, char *dst_ip) {
    int socks[NUM_CONN];
    char buffer[BUFFER_SIZE];
    uint64_t total = 0;
    time_t start = time(NULL);

    struct sockaddr_in server;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    for (int i = 1; i < NUM_CONN; i++) {
        socks[i] = socket(AF_INET, SOCK_STREAM, 0);
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(PORT_DOWNLOAD);
        inet_pton(AF_INET, SERVER_IP, &server.sin_addr);
        connect(socks[i], (struct sockaddr*)&server, sizeof(server));
    }
    
    struct sockaddr_in local_addr, remote_addr;
    getsockname(socks[0], (struct sockaddr *)&local_addr, &addr_len);
    getpeername(socks[0], (struct sockaddr *)&remote_addr, &addr_len);
    inet_ntop(AF_INET, &(local_addr.sin_addr), src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(remote_addr.sin_addr), dst_ip, INET_ADDRSTRLEN);

    while (time(NULL) - start < T) {
        for (int i = 0; i < NUM_CONN; i++) {
            int n = recv(socks[i], buffer, BUFFER_SIZE, MSG_DONTWAIT);
            if (n > 0) total += n;
        }
    }
    for (int i = 0; i < NUM_CONN; i++) close(socks[i]);
    printf("[✓] Descarga total: %lu bytes\n", total);
    return total;
}

void upload_test(uint32_t id) {
    char payload[BUFFER_SIZE];
    memset(payload, 'U', BUFFER_SIZE);
    time_t start;

    for (int i = 0; i < NUM_CONN; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in server;
            server.sin_family = AF_INET;
            server.sin_port = htons(PORT_UPLOAD);
            inet_pton(AF_INET, SERVER_IP, &server.sin_addr);
            connect(sock, (struct sockaddr*)&server, sizeof(server));

            uint8_t header[6];
            uint32_t netid = htonl(id);
            memcpy(header, &netid, 4);
            header[4] = (i+1) >> 8;
            header[5] = (i+1) & 0xff;
            send(sock, header, 6, 0);

            start = time(NULL);
            while (time(NULL) - start < T) {
                send(sock, payload, BUFFER_SIZE, 0);
            }
            close(sock);
            exit(0);
        }
    }
    for (int i = 0; i < NUM_CONN; i++) wait(NULL);
    printf("[✓] Upload terminado\n");
}

void consultar_resultados(uint32_t id, struct BW_result* resultado) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT_DOWNLOAD);
    inet_pton(AF_INET, SERVER_IP, &server.sin_addr);

    uint32_t netid = htonl(id);
    sendto(sock, &netid, 4, 0, (struct sockaddr*)&server, sizeof(server));

    char buffer[600];
    struct timeval timeout = {5, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    int r = recv(sock, buffer, sizeof(buffer), 0);

    if (r > 0) {
        int ok = unpackResultPayload(resultado, buffer, r);
        if (ok > 0) {
            printf("[✓] Resultados recibidos para ID 0x%x\n", id);
        } else {
            fprintf(stderr, "[X] Error al parsear resultados\n");
        }
    } else {
        fprintf(stderr, "[X] No se recibieron resultados\n");
    }
    close(sock);
}

void exportar_json(uint64_t bw_down, struct BW_result resultado, double rtt_idle, double rtt_down, double rtt_up, const char *src_ip, const char *dst_ip) {    
    printf("\n{\n");
    printf("  \"src_ip\": \"%s\",\n", src_ip);
    printf("  \"dst_ip\": \"%s\",\n", dst_ip);
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[20]; // "YYYY-MM-DD HH:MM:SS" son 19 + '\0'
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("  \"timestamp\": \"%s\",\n", timestamp);
    printf("  \"avg_bw_download_bps\": %lu,\n", bw_down);

    // Calcular avg upload bps
    double total = 0;
    for (int i = 0; i < NUM_CONN; i++) {
        if (resultado.conn_duration[i] > 0)
            total += (resultado.conn_bytes[i] * 8.0) / resultado.conn_duration[i];
    }
    printf("  \"avg_bw_upload_bps\": %.0f,\n", total / NUM_CONN);
    printf("  \"num_conns\": %d,\n", NUM_CONN);
    printf("  \"rtt_idle\": %.3f,\n", rtt_idle);
    printf("  \"rtt_download\": %.3f,\n", rtt_down);
    printf("  \"rtt_upload\": %.3f\n", rtt_up);
    printf("}\n\n");
}
