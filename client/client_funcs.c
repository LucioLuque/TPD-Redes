#include "client_funcs.h"

int parse_arguments(int argc, char *argv[], const char **ip_servidor, int *num_conn,
    bool *idle, bool *test_download, bool *test_upload, bool *json, const char **ip_hostremoto, int *json_port) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc) {
            *ip_servidor = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "-conn") == 0 && i + 1 < argc) {
            int conn = atoi(argv[i + 1]);
            if (conn > 0 && conn <= NUM_CONN_MAX) {
                *num_conn = conn;
            } else {
                fprintf(stderr, "[X] Número de conexiones inválido. Debe ser entre 1 y %d.\n", NUM_CONN_MAX);
                return 1;
            }
            i++;

        } else if (strcmp(argv[i], "-idle") == 0 && i + 1 < argc) {
            *idle = (strcmp(argv[i + 1], "true") == 0);
            i++;
        } else if (strcmp(argv[i], "-download") == 0 && i + 1 < argc) {
            *test_download = (strcmp(argv[i + 1], "true") == 0);
            i++;
        } else if (strcmp(argv[i], "-upload") == 0 && i + 1 < argc) {
            *test_upload = (strcmp(argv[i + 1], "true") == 0);
            i++;
        } else if (strcmp(argv[i], "-json") == 0 && i + 1 < argc) {
            *json = (strcmp(argv[i + 1], "true") == 0);
            i++;
        } else if (strcmp(argv[i], "-hostremoto") == 0 && i + 1 < argc) {
            *ip_hostremoto = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-jsonport") == 0 && i + 1 < argc) {
            int port = atoi(argv[i + 1]);
            if (port > 0 && port <= 65535) {
                *json_port = port;
            } else {
                fprintf(stderr, "[X] Puerto JSON inválido. Debe ser entre 1 y 65535.\n");
                return 1;
            }
            i++;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            fprintf(stderr, "Uso: %s -ip <IP_SERVIDOR> [-idle true|false] [-download true|false] [-upload true|false] [-conn <num_conn>] [-json true|false] [-hostremoto <IP_HOSTREMOTO>] [-jsonport <PUERTO_JSON>]\n", argv[0]);
            return 1;
        } else {
            fprintf(stderr, "Uso: %s -ip <IP_SERVIDOR> [-idle true|false] [-download true|false] [-upload true|false] [-conn <num_conn>]\n", argv[0]);
            return 1;
        }
    }

    if (*ip_servidor == NULL || strlen(*ip_servidor) == 0) {
        *ip_servidor = SERVER_IP;
    }

    if (*ip_hostremoto == NULL || strlen(*ip_hostremoto) == 0) {
        *ip_hostremoto = *ip_servidor; // Si no se especifica, usar el mismo servidor
    }

    return 0;
}

double calculate_rtt(const char *server_ip) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server, src;
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

    int r = recvfrom(sock, probe_recv, 4, 0, (struct sockaddr*)&src, &len);
    gettimeofday(&end, NULL);
    close(sock);

    if (r != 4) {
        fprintf(stderr, "[X] Error: respuesta inválida o incompleta. r=%d\n", r);
        return -1;
    }

    char ip_recv[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &src.sin_addr, ip_recv, sizeof(ip_recv));
    if (strcmp(ip_recv, server_ip) != 0) {
        fprintf(stderr, "[X] Error: IP recibida (%s) no coincide con la esperada (%s).\n", ip_recv, server_ip);
        return -1;
    }

    if (memcmp(probe_send, probe_recv, 4) != 0) {
        fprintf(stderr, "[X] Error: los datos recibidos no coinciden con lo enviado.\n");
        return -1;
    }

    double rtt = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1e6;
    return rtt;
}

double rtt_test(const char *server_ip, const char *etapa_nombre) {
    double total = 0.0;

    for (int i = 1; i <= 3; i++) {
        double rtt = calculate_rtt(server_ip);
        if (rtt < 0) {
            fprintf(stderr, "[X] Error al medir RTT (sin respuesta en 10 s) en etapa: %s, intento %d\n", etapa_nombre, i);
            return -1;
        }
        
        total += rtt;
           
        printf("[✓] RTT %s %d: %.4f segundos\n", etapa_nombre, i, rtt);
        if (i < 3) sleep(1);
    }

    double promedio = total / 3.0;
    printf("[✓] RTT %s promedio: %.4f segundos\n", etapa_nombre, promedio);
    return promedio;
}

uint32_t generate_id() { 
    uint32_t id = (uint32_t)random();
    while (id == 0 || (id >> 24) == 0xff) {
        id = (uint32_t)random();
    }
    return id;
}

// double download_test(const char *server_ip, char *src_ip, int num_conn) {
//     int pipes[num_conn][2];
//     pid_t pids[num_conn];
//     struct sockaddr_in server;
//     socklen_t addr_len = sizeof(server);

//     memset(&server, 0, sizeof(server));
//     server.sin_family = AF_INET;
//     server.sin_port   = htons(PORT_DOWNLOAD);
//     inet_pton(AF_INET, server_ip, &server.sin_addr);

//     // Para cada conexión: crea pipe, socket + fork
//     for (int i = 0; i < num_conn; i++) {
//         if (pipe(pipes[i]) < 0) {
//             perror("pipe");
//             exit(1);
//         }

//         int sock = socket(AF_INET, SOCK_STREAM, 0);
//         if (sock < 0) {
//             perror("socket");
//             exit(1);
//         }
//         if (connect(sock, (struct sockaddr*)&server, addr_len) < 0) {
//             perror("connect");
//             exit(1);
//         }

//         if ((pids[i] = fork()) == 0) {
//             // hijo i
//             close(pipes[i][0]);    // cierra lectura
//             uint64_t total_i = 0;
//             char buf[DATA_BUFFER_SIZE];
//             struct timeval start, now;
//             gettimeofday(&start, NULL);

//             while (1) {
//                 gettimeofday(&now, NULL);
//                 double elapsed = (now.tv_sec  - start.tv_sec)
//                                + (now.tv_usec - start.tv_usec) / 1e6;
//                 if (elapsed >= T) break;

//                 ssize_t n = recv(sock, buf, sizeof(buf), 0);
//                 if (n <= 0) break;
//                 total_i += n;
//             }

//             // escribe al padre
//             if (write(pipes[i][1], &total_i, sizeof(total_i)) < 0)
//                 perror("write pipe");
//             close(pipes[i][1]);
//             close(sock);
//             exit(0);
//         } else {
//             // padre
//             close(pipes[i][1]);   // cierra escritura
//             close(sock);          // padre no usa este socket
//         }
//     }

//     // El padre mide el tiempo total
//     struct timeval start_all, end_all;
//     gettimeofday(&start_all, NULL);

//     for (int i = 0; i < num_conn; i++) {
//         waitpid(pids[i], NULL, 0);
//     }

//     gettimeofday(&end_all, NULL);
//     double elapsed = (end_all.tv_sec  - start_all.tv_sec)
//                    + (end_all.tv_usec - start_all.tv_usec) / 1e6;

//     // Suma los bytes llegados de cada hijo
//     uint64_t total = 0;
//     for (int i = 0; i < num_conn; i++) {
//         uint64_t part = 0;
//         if (read(pipes[i][0], &part, sizeof(part)) < 0)
//             perror("read pipe");
//         close(pipes[i][0]);
//         total += part;
//     }

//     //obtener src_ip de la conexión
//     {
//       int tmp = socket(AF_INET, SOCK_STREAM, 0);
//       struct sockaddr_in local;
//       socklen_t len = sizeof(local);
//       connect(tmp, (struct sockaddr*)&server, addr_len);
//       getsockname(tmp, (struct sockaddr*)&local, &len);
//       inet_ntop(AF_INET, &local.sin_addr, src_ip, INET_ADDRSTRLEN);
//       close(tmp);
//     }

//     printf("[✓] Descarga total: %lu bytes en %.3f s\n", total, elapsed);
//     return (double)total * 8.0 / elapsed;  // bps reales
// }

double download_test(const char *server_ip, char *src_ip, int num_conn) {
    int pipes[num_conn][2];
    pid_t pids[num_conn];
    struct sockaddr_in server;
    socklen_t addr_len = sizeof(server);

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port   = htons(PORT_DOWNLOAD);
    inet_pton(AF_INET, server_ip, &server.sin_addr);

    // Para cada conexión: crea pipe, socket + fork
    for (int i = 0; i < num_conn; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            exit(1);
        }

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket");
            exit(1);
        }
        if (connect(sock, (struct sockaddr*)&server, addr_len) < 0) {
            perror("connect");
            exit(1);
        }

        if ((pids[i] = fork()) == 0) {
            // hijo i
            close(pipes[i][0]);    // cierra lectura
            uint64_t total_i = 0;
            char buf[DATA_BUFFER_SIZE];
            struct timeval start;
            gettimeofday(&start, NULL);
            
            ssize_t n;
            while ((n = recv(sock, buf, sizeof(buf), 0)) > 0) {
                total_i += n;
            }
            // escribe al padre
            if (write(pipes[i][1], &total_i, sizeof(total_i)) < 0)
                perror("write pipe");
            close(pipes[i][1]);
            close(sock);
            exit(0);
        } else {
            // padre
            close(pipes[i][1]);   // cierra escritura
            close(sock);          // padre no usa este socket
        }
    }

    // El padre mide el tiempo total
    struct timeval start_all, end_all;
    gettimeofday(&start_all, NULL);

    for (int i = 0; i < num_conn; i++) {
        waitpid(pids[i], NULL, 0);
    }

    gettimeofday(&end_all, NULL);
    double elapsed = (end_all.tv_sec  - start_all.tv_sec)
                   + (end_all.tv_usec - start_all.tv_usec) / 1e6;

    // Suma los bytes llegados de cada hijo
    uint64_t total = 0;
    for (int i = 0; i < num_conn; i++) {
        uint64_t part = 0;
        if (read(pipes[i][0], &part, sizeof(part)) < 0)
            perror("read pipe");
        close(pipes[i][0]);
        total += part;
    }

    //obtener src_ip de la conexión
    {
      int tmp = socket(AF_INET, SOCK_STREAM, 0);
      struct sockaddr_in local;
      socklen_t len = sizeof(local);
      connect(tmp, (struct sockaddr*)&server, addr_len);
      getsockname(tmp, (struct sockaddr*)&local, &len);
      inet_ntop(AF_INET, &local.sin_addr, src_ip, INET_ADDRSTRLEN);
      close(tmp);
    }

    printf("[✓] Descarga total: %lu bytes en %.3f s\n", total, elapsed);
    return (double)total * 8.0 / elapsed;  // bps reales
}

void upload_test(const char *server_ip, uint32_t id, int num_conn) {
    char payload[DATA_BUFFER_SIZE];
    memset(payload, 'U', DATA_BUFFER_SIZE);

    for (int i = 0; i < num_conn; i++) {
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
                send(sock, payload, DATA_BUFFER_SIZE, 0);
                gettimeofday(&now, NULL);
                elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1e6;
            }
            
            close(sock);
            exit(0);
        }
    }
    for (int i = 0; i < num_conn; i++) wait(NULL);
    printf("[✓] Upload terminado\n");
}

void query_results_from_server(const char *ip, int udp_port, uint32_t id_measurement, struct BW_result *out_result, int num_conn) {
    struct sockaddr_in servaddr;
    int sockfd;
    socklen_t len;
    char buffer[RESULT_BUFFER_SIZE];

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
    ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&servaddr, &len);
    if (n < 0) {
        perror("recvfrom");
        exit(EXIT_FAILURE);
    }

    // Desempaquetar con la funcion de handle_result
    if (unpackResultPayload(out_result, buffer, n) < 0) {
        fprintf(stderr, "Error desempaquetando resultados\n");
        close(sockfd);
        return;
    }

    //imprimir para verificar
    printf("Resultados de medición ID %u:\n", out_result->id_measurement);
    for (int i = 0; i < num_conn; i++) {
        if (out_result->conn_duration[i] > 0) {
            double throughput = (double)out_result->conn_bytes[i] * 8.0 / out_result->conn_duration[i];
            printf("Conexión %d: %.2f bytes/s (duración %.3f s)\n", i, throughput, out_result->conn_duration[i]);
        }
    }

    close(sockfd);
}

void export_json(uint64_t bw_down, uint64_t bw_up, double rtt_idle, double rtt_down, double rtt_up, const char *src_ip, const char *dst_ip, int num_conn, const char *ip_hostremoto, int json_port) {  
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[20]; // "YYYY-MM-DD HH:MM:SS" son 19 + '\0'
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("\n{\n");
    printf("  \"src_ip\": \"%s\",\n", src_ip);
    printf("  \"dst_ip\": \"%s\",\n", dst_ip);    
    printf("  \"timestamp\": \"%s\",\n", timestamp);
    printf("  \"avg_bw_download_bps\": %lu,\n", bw_down);
    printf("  \"avg_bw_upload_bps\": %lu,\n", bw_up);
    printf("  \"num_conns\": %d,\n", num_conn);
    printf("  \"rtt_idle\": %.4f,\n", rtt_idle);
    printf("  \"rtt_download\": %.4f,\n", rtt_down);
    printf("  \"rtt_upload\": %.4f\n", rtt_up);
    printf("}\n\n");

    char json_buffer[1024];
    int len = snprintf(json_buffer, sizeof(json_buffer),
        "{\n"
        "  \"src_ip\": \"%s\",\n"
        "  \"dst_ip\": \"%s\",\n"
        "  \"timestamp\": \"%s\",\n"
        "  \"avg_bw_download_bps\": %lu,\n"
        "  \"avg_bw_upload_bps\": %lu,\n"
        "  \"num_conns\": %d,\n"
        "  \"rtt_idle\": %.4f,\n"
        "  \"rtt_download\": %.4f,\n"
        "  \"rtt_upload\": %.4f\n"
        "}\n",
        src_ip, dst_ip, timestamp, bw_down, bw_up, num_conn, rtt_idle, rtt_down, rtt_up);

    if (len < 0 || len >= (int)sizeof(json_buffer)) {
        fprintf(stderr, "[X] Error al crear el JSON\n");
        return;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket UDP JSON");
        return;
    }

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(json_port);
    if (inet_pton(AF_INET, ip_hostremoto, &dest.sin_addr) <= 0) {
        perror("inet_pton JSON");
        close(sock);
        return;
    }

    if (sendto(sock, json_buffer, len, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
        perror("sendto JSON");
    } else {
        printf("[✓] JSON enviado a %s:%d\n", ip_hostremoto, PORT_DOWNLOAD);
    }
    close(sock);
}