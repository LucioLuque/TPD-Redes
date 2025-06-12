
#include "server_funcs.h"
struct ResultEntry *results = NULL;

pthread_mutex_t results_mutex = PTHREAD_MUTEX_INITIALIZER;

struct BW_result *get_or_create_result(uint32_t id_measurement) {
    pthread_mutex_lock(&results_mutex);
    struct ResultEntry *actual = results;
    while (actual) {
        if (actual->result.id_measurement == id_measurement) {
            pthread_mutex_unlock(&results_mutex);
            return &actual->result;
        }
        actual = actual->next;
    }
    struct ResultEntry *new = malloc(sizeof(struct ResultEntry));
    new->result.id_measurement = id_measurement;
    memset(new->result.conn_bytes, 0, sizeof(new->result.conn_bytes));
    memset(new->result.conn_duration, 0, sizeof(new->result.conn_duration));
    new->next = results;
    results = new;
    pthread_mutex_unlock(&results_mutex);
    return &new->result;
}



void *handle_download_conn(void *arg) {
    struct thread_arg_t *args = arg;
    int sock = args->client_sock;
    struct sockaddr_in client_addr = args->client_addr;
    free(arg);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    printf("[+] Nueva conexión de descarga desde %s\n", client_ip);

    char buffer[DATA_BUFFER_SIZE];
    memset(buffer, 'D', sizeof(buffer));

    struct timeval start, now;
    gettimeofday(&start, NULL);

    bool timed_out = false;
    bool peer_closed = false;

    // 1) Envío hasta T+3 segundos o hasta que el cliente cierre
    while (1) {
        // Chequeo tiempo
        gettimeofday(&now, NULL);
        double elapsed = (now.tv_sec - start.tv_sec)
                       + (now.tv_usec - start.tv_usec) / 1e6;
        if (elapsed >= T + 3) {
            timed_out = true;
            break;
        }

        // Compruebo FIN del cliente con select+peek
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        struct timeval tv = { 0, 0 };
        int r = select(sock + 1, &rfds, NULL, NULL, &tv);
        if (r > 0 && FD_ISSET(sock, &rfds)) {
            char tmp;
            ssize_t rc = recv(sock, &tmp, 1, MSG_PEEK | MSG_DONTWAIT);
            if (rc == 0) {
                peer_closed = true;
                break;
            } else if (rc < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv peek");
                peer_closed = true;
                break;
            }
        }

        // Envío datos
        ssize_t sent = send(sock, buffer, sizeof(buffer), 0);
        if (sent < 0) {
            fprintf(stderr,
                    "[!] Error al enviar a %s (sock=%d): %s\n",
                    client_ip, sock, strerror(errno));
            peer_closed = true;
            break;
        }
    }

    // 2) Decido cómo notificar el cierre
    if (timed_out) {
        if (shutdown(sock, SHUT_WR) < 0) {
            perror("shutdown SHUT_WR");
        } else {
            printf("[✓] FIN forzado por timeout a %s\n", client_ip);
        }
    } else if (peer_closed) {
        printf("[✓] Cliente cerró primero, fin de envío a %s\n", client_ip);
    }

    // 3) Cierro socket y finalizo hilo
    close(sock);
    pthread_exit(NULL);
}

// void *handle_download_conn(void *arg) {
//     struct thread_arg_t *args = (struct thread_arg_t *)arg;
//     int sock = args->client_sock;
//     struct sockaddr_in client_addr = args->client_addr;
//     free(arg);

//     char client_ip[INET_ADDRSTRLEN];
//     inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
//     printf("[+] Nueva conexion de descarga desde %s\n", client_ip);

//     char buffer[DATA_BUFFER_SIZE];
//     memset(buffer, 'D', DATA_BUFFER_SIZE);

//     struct timeval start, now;
//     gettimeofday(&start, NULL);
//     double elapsed = 0.0;

//     while (elapsed < T + 3) {
//         ssize_t sent = send(sock, buffer, DATA_BUFFER_SIZE, 0);
//         if (sent <= 0) break;  // error o cliente cerro conexion

//         gettimeofday(&now, NULL);
//         elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1e6;
//     }
//     printf("[✓] Conexion de descarga cerrada desde %s\n", client_ip);
//     close(sock);
//     pthread_exit(NULL);
// }


// void *handle_download_conn(void *arg) {
//     struct thread_arg_t *args = (struct thread_arg_t *)arg;
//     int sock = args->client_sock;
//     struct sockaddr_in client_addr = args->client_addr;
//     free(arg);

//     char client_ip[INET_ADDRSTRLEN];
//     inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
//     printf("[+] Nueva conexion de descarga desde %s\n", client_ip);

//     char buffer[DATA_BUFFER_SIZE];
//     memset(buffer, 'D', DATA_BUFFER_SIZE);

//     struct timeval start, now;
//     gettimeofday(&start, NULL);
//     double elapsed = 0.0;

//     while (elapsed < T + 3) {
//         ssize_t sent = send(sock, buffer, DATA_BUFFER_SIZE, MSG_NOSIGNAL);
                            
//         if (sent < 0) {
//             if (errno == EPIPE) {
//                 fprintf(stderr,
//                     "[!] EPIPE: el cliente cerró la conexión (sock=%d)\n",
//                     sock);
//             } else {
//                 perror("send");
//             }
//             break;
//         }
    
//         gettimeofday(&now, NULL);
//         elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1e6;
//     }
//     printf("[✓] Conexion de descarga cerrada desde %s\n", client_ip);
//     close(sock);
//     pthread_exit(NULL);
// }

void *handle_upload_conn(void *arg) {
    int sock = *(int*)arg;
    free(arg);
    uint8_t header[6];
    if (recv(sock, header, 6, MSG_WAITALL) != 6) {
        close(sock);
        pthread_exit(NULL);
    }
    uint32_t id_measurement;
    memcpy(&id_measurement, header, 4);
    id_measurement = ntohl(id_measurement);
    uint16_t id_conn = (header[4] << 8) | header[5];

    struct BW_result *res = get_or_create_result(id_measurement);

    char buffer[DATA_BUFFER_SIZE];
    struct timeval start, now;
    gettimeofday(&start, NULL);
    uint64_t total_bytes = 0;
    while (1) {
        int n = recv(sock, buffer, DATA_BUFFER_SIZE, 0);
        if (n <= 0) break;
        total_bytes += n;

        gettimeofday(&now, NULL);
        double elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1e6;
        if (elapsed >= T) break;  // Si ya pasaron T segundos se cierra
    }
    if (id_conn > 0 && id_conn <= NUM_CONN_MAX) {
        double duration = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1e6;
        res->conn_bytes[id_conn - 1] = total_bytes;
        res->conn_duration[id_conn - 1] = duration;
        printf("[+] Upload registrado: ID 0x%x, Conn %d, Bytes: %" PRIu64 ", Duracion: %.3f\n",
            id_measurement, id_conn,
            total_bytes,
            duration);
    }
    close(sock);
    pthread_exit(NULL);
}

void *udp_server_thread(void *arg) {
    (void)arg; // evita warning de variable no usada
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serv, cli;
    socklen_t len = sizeof(cli);
    char buffer[DATA_BUFFER_SIZE];
    bool found;
    struct BW_result tmp_result;
    int n;

    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT_DOWNLOAD);
    serv.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("bind UDP");
        close(sock);
        return NULL;
    }

    printf("[+] UDP servidor escuchando en puerto %d\n", PORT_DOWNLOAD);

    while (1) {
        n = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&cli, &len);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }
        // RTT
        if (n == 4 && (unsigned char)buffer[0] == 0xff) {
            sendto(sock, buffer, 4, 0, (struct sockaddr*)&cli, len);
            continue;
        }
        //consultar resultados
        if (n == 4) {
            uint32_t id;
            memcpy(&id, buffer, 4);
            id = ntohl(id);
            printf("[?] Consulta recibida para ID 0x%x\n", id);
           
            found = false;

            pthread_mutex_lock(&results_mutex);
            struct ResultEntry *r = results;
            while (r && r->result.id_measurement != id) r = r->next;
            if (r) {
                tmp_result = r->result;
                found = true;
            }
            pthread_mutex_unlock(&results_mutex);
            if (found) {

                printf("[✓] ID encontrado. Enviando resultados\n");
                char response[RESULT_BUFFER_SIZE];
                int resp_size = packResultPayload(tmp_result, response, sizeof(response));
                sendto(sock, response, resp_size, 0, (struct sockaddr*)&cli, len);
            }
            else {
                // No se encontro el ID
                fprintf(stderr,
                    "[!] ID no encontrado: 0x%x (largo=%d, primer_byte=0x%02x)\n",
                    id, n, (unsigned char)buffer[0]);
            }
            continue;
        }

        //cualquier otro caso es invalido
        fprintf(stderr,
        "[!] UDP inválido: largo=%d, primer_byte=0x%02x\n", n, (n>0 ? (unsigned char)buffer[0] : 0));
        
    }
    close(sock);
    return NULL;
}