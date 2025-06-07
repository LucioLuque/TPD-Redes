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
#include <sys/time.h>   // Para gettimeofday

#include "../handle_result/handle_result.h"

#define SERVER_IP "127.0.0.1"
#define PORT_DOWNLOAD 20251
#define PORT_UPLOAD 20252
#define BUFFER_SIZE 1024
#define NUM_CONN 10
#define T 20

uint32_t generate_id();
double medir_rtt();
uint64_t download_test(const char *server_ip, char *src_ip, char *dst_ip);
void upload_test(const char *server_ip, uint32_t id);
void consultar_resultados(const char *ip, int udp_port, uint32_t id_measurement, struct BW_result *out_result);
void exportar_json(uint64_t bw_down, struct BW_result resultado, double rtt_idle, double rtt_down, double rtt_up, const char *src_ip, const char *dst_ip);

double get_time_now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <IP_SERVIDOR>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip_servidor = argv[1];
    //si esta vacia poner SERVER_IP
    if (strlen(ip_servidor) == 0) {
        ip_servidor = SERVER_IP;
    }
    printf("[+] Conectando al servidor %s\n", ip_servidor);
    // Paso 1: medir latencia antes de todo (fase idle)
    sleep(1);
    double rtt_idle = medir_rtt(ip_servidor);
    if (rtt_idle < 0) exit(1);

    // Paso 2: download test
    double rtt_down = medir_rtt(ip_servidor);
    char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
    uint64_t total_bytes = download_test(ip_servidor, src_ip, dst_ip);
    double bw_download_bps = (double)total_bytes * 8 / T;

    // Paso 3: upload test
    double rtt_up = medir_rtt(ip_servidor);
    uint32_t id = generate_id();
    upload_test(ip_servidor, id);

    // Paso 4: consultar resultados
    struct BW_result resultado;

    int puerto_udp = PORT_DOWNLOAD;
    puerto_udp = PORT_DOWNLOAD;
    sleep(1); // Esperar un segundo antes de consultar resultados
    consultar_resultados(ip_servidor, puerto_udp, id, &resultado);

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

double medir_rtt(const char *server_ip) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server;
    socklen_t len = sizeof(server);

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT_DOWNLOAD);  // UDP también usa 20251
    inet_pton(AF_INET, server_ip, &server.sin_addr);

    uint8_t probe_send[4] = {0xff, rand() % 256, rand() % 256, rand() % 256};
    uint8_t probe_recv[4];

    struct timeval timeout = {10, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct timeval start, end;

    gettimeofday(&start, NULL);
    int sent = sendto(sock, probe_send, 4, 0, (struct sockaddr*)&server, len);
    if (sent != 4) {
        perror("sendto");
        close(sock);
        return -1;
    }

    int r = recvfrom(sock, probe_recv, 4, 0, (struct sockaddr*)&server, &len);
    gettimeofday(&end, NULL);
    close(sock);

    if (r != 4) {
        fprintf(stderr, "[X] Error: respuesta inválida o incompleta. r=%d\n", r);
        return -1;
    }

    if (memcmp(probe_send, probe_recv, 4) != 0) {
        fprintf(stderr, "[X] Error: los datos recibidos no coinciden con lo enviado.\n");
        return -1;
    }

    double rtt = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1e6;
    printf("[✓] RTT: %.3f segundos\n", rtt);
    sleep(1);
    return rtt;
}



uint64_t download_test(const char *server_ip, char *src_ip, char *dst_ip) {
    int socks[NUM_CONN];
    char buffer[BUFFER_SIZE];
    uint64_t total = 0;
    time_t start = time(NULL);

    struct sockaddr_in server;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    for (int i = 0; i < NUM_CONN; i++) {
        socks[i] = socket(AF_INET, SOCK_STREAM, 0);
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(PORT_DOWNLOAD);
        inet_pton(AF_INET, server_ip, &server.sin_addr);
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

void upload_test(const char *server_ip, uint32_t id) {
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
            inet_pton(AF_INET, server_ip, &server.sin_addr);
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

void consultar_resultados(const char *ip, int udp_port, uint32_t id_measurement, struct BW_result *out_result) {
    struct sockaddr_in servaddr;
    int sockfd;
    socklen_t len;
    char buffer[2048];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(udp_port);
    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    // Enviar el ID de medición al servidor
    uint32_t id_net = htonl(id_measurement);
    if (sendto(sockfd, &id_net, sizeof(id_net), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }

    // Recibir los resultados
    len = sizeof(servaddr);
    ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&servaddr, &len);
    if (n < 0) {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }

    // Desempaquetar
    if (unpackResultPayload(out_result, buffer, n) < 0) {
        fprintf(stderr, "Error desempaquetando resultados\n");
        close(sockfd);
        return;
    }

    // Imprimir los resultados
    printf("Resultados de medición ID %u:\n", out_result->id_measurement);
    for (int i = 0; i < NUM_CONN; i++) {
        if (out_result->conn_duration[i] > 0) {
            double throughput = (double)out_result->conn_bytes[i] * 8.0 / out_result->conn_duration[i];
            printf("Conexión %d: %.2f bytes/s (duración %.3f s)\n", i, throughput, out_result->conn_duration[i]);
        }
    }

    close(sockfd);
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
