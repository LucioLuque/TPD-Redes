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

#include "../handle_result/handle_result.h"

#define PORT_DOWNLOAD 20251
#define PORT_UPLOAD 20252
#define MAX_CLIENTS 100
#define BUFFER_SIZE 4028
#define T 20  // segundos


// Estructura para almacenar resultados de medicion
struct ResultadoEntry {
    struct BW_result result;
    struct ResultadoEntry *next;
};

struct ResultadoEntry *resultados = NULL;
pthread_mutex_t resultados_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_download_conn(void *arg);
void *handle_upload_conn(void *arg);
void *udp_server_thread(void *arg);

// Buscar o crear entrada para ID de medicion
struct BW_result *obtener_o_crear_resultado(uint32_t id_measurement) {
    pthread_mutex_lock(&resultados_mutex);
    struct ResultadoEntry *actual = resultados;
    while (actual) {
        if (actual->result.id_measurement == id_measurement) {
            pthread_mutex_unlock(&resultados_mutex);
            return &actual->result;
        }
        actual = actual->next;
    }
    struct ResultadoEntry *nuevo = malloc(sizeof(struct ResultadoEntry));
    nuevo->result.id_measurement = id_measurement;
    memset(nuevo->result.conn_bytes, 0, sizeof(nuevo->result.conn_bytes));
    memset(nuevo->result.conn_duration, 0, sizeof(nuevo->result.conn_duration));
    nuevo->next = resultados;
    resultados = nuevo;
    pthread_mutex_unlock(&resultados_mutex);
    return &nuevo->result;
}

struct thread_arg_t {
    int client_sock;
    struct sockaddr_in client_addr;
};

int main() {
    signal(SIGPIPE, SIG_IGN);
    int tcp_sock_down, tcp_sock_up;
    struct sockaddr_in server_addr;

    tcp_sock_down = socket(AF_INET, SOCK_STREAM, 0);
    tcp_sock_up   = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock_down < 0 || tcp_sock_up < 0) {
        perror("socket"); exit(EXIT_FAILURE);
    }

    int yes = 1;
    if (setsockopt(tcp_sock_down, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0 ||
        setsockopt(tcp_sock_up, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt"); exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // TCP Download
    server_addr.sin_port = htons(PORT_DOWNLOAD);
    if (bind(tcp_sock_down, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind de download");
        return 1;
    }

    if (listen(tcp_sock_down, MAX_CLIENTS) < 0) {
        perror("Error en listen de download");
        return 1;
    }

    // TCP Upload
    server_addr.sin_port = htons(PORT_UPLOAD);
    if (bind(tcp_sock_up, (struct sockaddr*)&server_addr, sizeof(server_addr))<0) {
        perror("Error en bind de upload");
        return 1;
    }
    
    if (listen(tcp_sock_up, MAX_CLIENTS) < 0) {
        perror("Error en listen de upload");
        return 1;
    }

    // UDP server thread
    pthread_t udp_tid;
    pthread_create(&udp_tid, NULL, udp_server_thread, NULL);

    printf("[+] Servidor iniciado. Escuchando TCP y UDP...\n");

    // Loop de aceptación de conexiones
    // tiene que cortar? es decir el server siempre espera?
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(tcp_sock_down, &read_fds);
        FD_SET(tcp_sock_up, &read_fds);
        int maxfd = (tcp_sock_down > tcp_sock_up) ? tcp_sock_down : tcp_sock_up;

        select(maxfd + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(tcp_sock_down, &read_fds)) {
            struct sockaddr_in client;
            socklen_t len = sizeof(client);
            int client_sock = accept(tcp_sock_down, (struct sockaddr*)&client, &len);
            pthread_t tid;

            // int *arg = malloc(sizeof(int) + sizeof(struct sockaddr_in));
            // memcpy(arg, &client_sock, sizeof(int));
            // memcpy((char*)arg + sizeof(int), &client, sizeof(struct sockaddr_in));
            // pthread_create(&tid, NULL, handle_download_conn, arg);
            struct thread_arg_t *arg = malloc(sizeof(struct thread_arg_t));
            arg->client_sock = client_sock;
            arg->client_addr = client;
            pthread_create(&tid, NULL, handle_download_conn, arg);
        }

        if (FD_ISSET(tcp_sock_up, &read_fds)) {
            struct sockaddr_in client;
            socklen_t len = sizeof(client);
            int client_sock = accept(tcp_sock_up, (struct sockaddr*)&client, &len);
            pthread_t tid;
            int *arg = malloc(sizeof(int)); *arg = client_sock;
            pthread_create(&tid, NULL, handle_upload_conn, arg);
        }
    }
    
    return 0;
}

void *handle_download_conn(void *arg) {
    struct thread_arg_t *args = (struct thread_arg_t *)arg;
    int sock = args->client_sock;
    struct sockaddr_in client_addr = args->client_addr;
    free(arg);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("[+] Nueva conexion de descarga desde %s\n", client_ip);

    char buffer[BUFFER_SIZE];
    memset(buffer, 'D', BUFFER_SIZE);
    struct timeval start, now;
    gettimeofday(&start, NULL);
    double elapsed = 0.0;
    while (elapsed < T + 3) {
        ssize_t sent = send(sock, buffer, BUFFER_SIZE, 0);
        if (sent <= 0) break;  // error o cliente cerro conexion

        gettimeofday(&now, NULL);
        elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1e6;

        // Si pasaron T segundos y no se envio nada, se cierra
        if (elapsed >= T && sent == 0) break;
    }
    printf("[✓] Conexion de descarga cerrada desde %s\n", client_ip);
    close(sock);
    pthread_exit(NULL);
}

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

    struct BW_result *res = obtener_o_crear_resultado(id_measurement);

    char buffer[BUFFER_SIZE];
    struct timeval start, now;
    gettimeofday(&start, NULL);
    uint64_t total_bytes = 0;
    while (1) {
        int n = recv(sock, buffer, BUFFER_SIZE, 0);
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
    (void)arg; // sacar despues esto
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serv, cli;
    socklen_t len = sizeof(cli);
    char buffer[BUFFER_SIZE];
    bool found;
    struct BW_result tmp_result;

    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT_DOWNLOAD);
    serv.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr*)&serv, sizeof(serv));

    printf("[+] UDP servidor escuchando en puerto %d\n", PORT_DOWNLOAD);

    while (1) {
        int n = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&cli, &len);
        if (n == 4 && (unsigned char)buffer[0] == 0xff) {
            sendto(sock, buffer, 4, 0, (struct sockaddr*)&cli, len);
        } else if (n == 4) {
            uint32_t id;
            memcpy(&id, buffer, 4);
            id = ntohl(id);
            printf("[?] Consulta recibida para ID 0x%x\n", id);
            
           
            found = false;

            pthread_mutex_lock(&resultados_mutex);
            struct ResultadoEntry *r = resultados;
            while (r && r->result.id_measurement != id) r = r->next;
            // pthread_mutex_unlock(&resultados_mutex);
            if (r) {
                tmp_result = r->result;
                found = true;
            }
            pthread_mutex_unlock(&resultados_mutex);
            if (found) {

                printf("[✓] ID encontrado. Enviando resultados\n");
                char response[10240];
                int resp_size = packResultPayload(tmp_result, response, sizeof(response));
                sendto(sock, response, resp_size, 0, (struct sockaddr*)&cli, len);
            }
            else {
                printf("[!] ID 0x%x no encontrado\n", id);
            }
        }
    }
    close(sock);
    return NULL;
}
