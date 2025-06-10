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

#include "../handle_result/handle_result.h"

// #define SERVER_IP "192.168.0.15"
#define SERVER_IP "127.0.0.1"
#define PORT_DOWNLOAD 20251
#define PORT_UPLOAD 20252
#define NUM_CONN_MAX 100
int NUM_CONN = 10;
int BUFFER_SIZE = (20 + 1 + 6 + 1) * NUM_CONN + 4; 


#define T 20

uint32_t generate_id();
double medir_rtt();
double download_test(const char *server_ip, char *src_ip, char *dst_ip);
void upload_test(const char *server_ip, uint32_t id);
void consultar_resultados(const char *ip, int udp_port, uint32_t id_measurement, struct BW_result **out_result);
void exportar_json(uint64_t bw_down, uint64_t bw_up, double rtt_idle, double rtt_down, double rtt_up, const char *src_ip, const char *dst_ip);

double medir_rtt_promedio_con_tres_intentos(const char *server_ip, const char *etapa_nombre) {
    double total = 0.0;
    int exitos = 0;

    for (int i = 1; i <= 3; i++) {
        double rtt = medir_rtt(server_ip);
        if (rtt >= 0) {
            total += rtt;
            exitos++;
            printf("[✓] RTT %s %d: %.3f segundos\n", etapa_nombre, i, rtt);
        } else {
            printf("[X] Falló RTT %s %d\n", etapa_nombre, i);
        }
        if (i < 3) sleep(1);
    }

    if (exitos == 0) {
        fprintf(stderr, "[X] Todas las mediciones de RTT fallaron para etapa: %s. Abortando.\n", etapa_nombre);
        exit(1);
    }

    double promedio = total / exitos;
    printf("[✓] RTT %s promedio: %.3f segundos\n", etapa_nombre, promedio);
    return promedio;
}

int parseo(int argc, char *argv[], const char **ip_servidor, bool *test_rtt, bool *test_download, bool *test_upload) {
    // Parseo de argumentos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-rtt") == 0 && i + 1 < argc) {
            *test_rtt = (strcmp(argv[i + 1], "true") == 0);
            i++;
        } else if (strcmp(argv[i], "-download") == 0 && i + 1 < argc) {
            *test_download = (strcmp(argv[i + 1], "true") == 0);
            i++;
        } else if (strcmp(argv[i], "-upload") == 0 && i + 1 < argc) {
            *test_upload = (strcmp(argv[i + 1], "true") == 0);
            i++;
        } else if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc) {
            *ip_servidor = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-conn") == 0 && i + 1 < argc) {
            int conn = atoi(argv[i + 1]);
            if (conn > 0 && conn <= NUM_CONN_MAX) {
                NUM_CONN = conn;
                BUFFER_SIZE = (20 + 1 + 6 + 1) * NUM_CONN + 4; // Recalcular el tamaño del buffer
            } else {
                fprintf(stderr, "[X] Número de conexiones inválido. Debe ser entre 1 y %d.\n", NUM_CONN_MAX);
                return 1;
            }
            i++;
        } else {
            fprintf(stderr, "Uso: %s -ip <IP_SERVIDOR> [-rtt true|false] [-download true|false] [-upload true|false] [-conn <NUM_CONN>]\n", argv[0]);
            return 1;
        }
    }

    if (*ip_servidor == NULL || strlen(*ip_servidor) == 0) {
        *ip_servidor = SERVER_IP;
    }
    return 0;
}


int main(int argc, char *argv[]) {
    const char *ip_servidor = NULL;
    bool test_rtt = true;
    bool test_download = true;
    bool test_upload = true;

    if (parseo(argc, argv, &ip_servidor, &test_rtt, &test_download, &test_upload)) {    
        return 1; // Error en el parseo
    }
    //imprimir todos los campos de parseo
    printf("IP del servidor: %s\n", ip_servidor);
    printf("Test de RTT: %s\n", test_rtt ? "Habilitado" : "Deshabilitado");
    printf("Test de descarga: %s\n", test_download ? "Habilitado" : "Deshabilitado");
    printf("Test de subida: %s\n", test_upload ? "Habilitado" : "Deshabilitado");
    printf("Número de conexiones: %d\n", NUM_CONN);
    printf("[+] Conectando al servidor %s\n", ip_servidor);

    double rtt_idle = 0.0, rtt_down = 0.0, rtt_up = 0.0;
    double bw_download_bps = 0.0, bw_upload_bps = 0.0;
    char src_ip[INET_ADDRSTRLEN] = "0.0.0.0";
    char dst_ip[INET_ADDRSTRLEN] = "0.0.0.0";
    uint32_t id = 0;

    // Paso 1: medir latencia antes de todo (fase idle)
    if (test_rtt) {
        rtt_idle = medir_rtt_promedio_con_tres_intentos(ip_servidor, "idle");
    } 
    if (test_download) {
        // Paso 2: download test
        rtt_down = 0.0; //deberiamos calcular esto bien!!!
        bw_download_bps = download_test(ip_servidor, src_ip, dst_ip);
    }
    
    if (test_upload) {
        // Paso 3: upload test
        id = generate_id();
        rtt_up = 0.0; //deberiamos calcular esto bien!!!
        upload_test(ip_servidor, id);

        // Paso 4: consultar resultados
        struct BW_result *resultado = NULL;
        sleep(1); // Esperar un segundo antes de consultar resultados xq 
                // el upload puede tardar un poco en completarse
        consultar_resultados(ip_servidor, PORT_DOWNLOAD, id, &resultado);
        if (!resultado) {
            fprintf(stderr, "[X] Error al consultar resultados.\n");
            return 1;
        }
        // Calcular avg upload bps
        double total = 0;
        for (int i = 0; i < NUM_CONN; i++) {
            if (resultado->conn_duration[i] > 0) {
                total += (resultado->conn_bytes[i] * 8.0) / resultado->conn_duration[i];
            }
        }
        bw_upload_bps = total / NUM_CONN;
        free_bw_result(resultado);
    }

    

    // Paso 5: exportar el JSON, por ahora manda por prints
    exportar_json(bw_download_bps, bw_upload_bps, rtt_idle, rtt_down, rtt_up, src_ip, dst_ip);
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
    server.sin_port = htons(PORT_DOWNLOAD);
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
    return rtt;
}

double download_test(const char *server_ip, char *src_ip, char *dst_ip) {
    int socks[NUM_CONN];
    // char buffer[BUFFER_SIZE];
    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memset(buffer, 0, BUFFER_SIZE);
    uint64_t total = 0;
    struct timeval start, end;

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

    gettimeofday(&start, NULL);
    double elapsed = 0.0;
    // while (elapsed < T) {
    //     for (int i = 0; i < NUM_CONN; i++) {
    //         if (socks[i] == -1) continue; // Si el socket está cerrado, saltar
    //         ssize_t n = recv(socks[i], buffer, BUFFER_SIZE, MSG_DONTWAIT);
    //         if (n < 0) {
    //             if (errno != EAGAIN && errno != EWOULDBLOCK) {
    //                 perror("[X] Error en recv");
    //                 close(socks[i]);
    //                 socks[i] = -1;
    //             }
    //         } else if (n == 0) {
    //             close(socks[i]);
    //             socks[i] = -1;
    //         } else {
    //             total += n;
    //         }
    //     }
    //     gettimeofday(&end, NULL);
    //     elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    // }
    while (elapsed < T) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int maxfd = -1;

        // Agregar solo sockets abiertos
        for (int i = 0; i < NUM_CONN; i++) {
            if (socks[i] != -1) {
                FD_SET(socks[i], &readfds);
                if (socks[i] > maxfd) maxfd = socks[i];
            }
        }

        struct timeval timeout = {0, 500000}; // 0.5s
        int activity = select(maxfd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0 && errno != EINTR) {

            perror("[X] select");

            break;
        }

        for (int i = 0; i < NUM_CONN; i++) {
            if (socks[i] != -1 && FD_ISSET(socks[i], &readfds)) {
                ssize_t n = recv(socks[i], buffer, BUFFER_SIZE, 0);
                if (n < 0) {
                    perror("[X] recv");
                    close(socks[i]);
                    socks[i] = -1;
                } else if (n == 0) {
                    // conexión cerrada por el servidor
                    close(socks[i]);
                    socks[i] = -1;
                } else {
                    total += n;
                }
            }
        }

        gettimeofday(&end, NULL);
        elapsed = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    }
    
    for (int i = 0; i < NUM_CONN; i++) close(socks[i]);
    printf("[✓] Descarga total: %lu bytes\n", total);
    double bw_bps = (double)total * 8.0 / elapsed; // Convertir a bits por segundo
    free(buffer);
    return bw_bps;
}

void upload_test(const char *server_ip, uint32_t id) {
    // char payload[BUFFER_SIZE];
    char *payload = malloc(BUFFER_SIZE);
    if (!payload) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memset(payload, 'U', BUFFER_SIZE);
    

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

            struct timeval start, now;
            gettimeofday(&start, NULL);
            double elapsed = 0.0;
            while (elapsed < T) {
                send(sock, payload, BUFFER_SIZE, 0);
                gettimeofday(&now, NULL);
                elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1e6;
            }
            
            close(sock);
            exit(0);
        }
    }
    for (int i = 0; i < NUM_CONN; i++) wait(NULL);
    printf("[✓] Upload terminado\n");
    free(payload);
    

}

void consultar_resultados(const char *ip, int udp_port, uint32_t id_measurement, struct BW_result **out_result) {
    struct sockaddr_in servaddr;
    int sockfd;
    socklen_t len;
    // char buffer[BUFFER_SIZE];
    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

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

    // Id al servidor
    uint32_t id_net = htonl(id_measurement);
    if (sendto(sockfd, &id_net, sizeof(id_net), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }

    // Recibir del server
    len = sizeof(servaddr);
    ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&servaddr, &len);
    
    if (n < 0) {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }

    int r = unpackResultPayload(out_result, buffer, n, NUM_CONN);
    free(buffer);
    if (r<0){
        fprintf(stderr, "[X] Error al desempaquetar resultados: %d\n", r);
        close(sockfd);
        return;
    }

    struct BW_result *result = *out_result;

    // por ahora imprimir para verificar
    printf("Resultados de medición ID %u:\n", result->id_measurement);
    for (int i = 0; i < NUM_CONN; i++) {
        if (result->conn_duration[i] > 0) {
            double throughput = (double)result->conn_bytes[i] * 8.0 / result->conn_duration[i];
            printf("Conexión %d: %.2f bytes/s (duración %.3f s)\n", i, throughput, result->conn_duration[i]);
        }
    }

    close(sockfd);
}


void exportar_json(uint64_t bw_down, uint64_t bw_up, double rtt_idle, double rtt_down, double rtt_up, const char *src_ip, const char *dst_ip) {    
    printf("\n{\n");
    printf("  \"src_ip\": \"%s\",\n", src_ip);
    printf("  \"dst_ip\": \"%s\",\n", dst_ip);
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[20]; // "YYYY-MM-DD HH:MM:SS" son 19 + '\0'
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("  \"timestamp\": \"%s\",\n", timestamp);
    printf("  \"avg_bw_download_bps\": %lu,\n", bw_down);

    printf("  \"avg_bw_upload_bps\": %lu,\n", bw_up);
    printf("  \"num_conns\": %d,\n", NUM_CONN);
    printf("  \"rtt_idle\": %.3f,\n", rtt_idle);
    printf("  \"rtt_download\": %.3f,\n", rtt_down);
    printf("  \"rtt_upload\": %.3f\n", rtt_up);
    printf("}\n\n");
}
