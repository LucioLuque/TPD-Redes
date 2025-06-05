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

#include "../handle_result/handle_result.h"

#define PORT_DOWNLOAD 20251
#define PORT_UPLOAD 20252
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define T 20  // segundos

// Estructura para guardar resultados por identificador de prueba
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

int main() {
    int tcp_sock_down, tcp_sock_up;
    struct sockaddr_in server_addr;

    tcp_sock_down = socket(AF_INET, SOCK_STREAM, 0);
    tcp_sock_up   = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock_down < 0 || tcp_sock_up < 0) {
        perror("socket"); exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(tcp_sock_down, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(tcp_sock_up, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind TCP Download
    server_addr.sin_port = htons(PORT_DOWNLOAD);
    bind(tcp_sock_down, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(tcp_sock_down, MAX_CLIENTS);

    // Bind TCP Upload
    server_addr.sin_port = htons(PORT_UPLOAD);
    bind(tcp_sock_up, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(tcp_sock_up, MAX_CLIENTS);

    // UDP handler thread
    pthread_t udp_tid;
    pthread_create(&udp_tid, NULL, udp_server_thread, NULL);

    printf("[+] Servidor iniciado. Escuchando TCP y UDP...\n");

    // Loop de aceptación de conexiones
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
            int *arg = malloc(sizeof(int)); *arg = client_sock;
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
    int sock = *(int*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    memset(buffer, 'D', BUFFER_SIZE);
    time_t start = time(NULL);
    while (time(NULL) - start < T) {
        if (send(sock, buffer, BUFFER_SIZE, 0) <= 0) break;
    }
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
    time_t start = time(NULL);
    int total_bytes = 0;
    while (time(NULL) - start < T) {
        int n = recv(sock, buffer, BUFFER_SIZE, 0);
        if (n <= 0) break;
        total_bytes += n;
    }
    if (id_conn > 0 && id_conn <= NUM_CONN) {
        res->conn_bytes[id_conn - 1] = total_bytes;
        res->conn_duration[id_conn - 1] = difftime(time(NULL), start);
        printf("[+] Upload registrado: ID 0x%x, Conn %d, Bytes: %d, Duracion: %.3f\n",
            id_measurement, id_conn,
            total_bytes,
            difftime(time(NULL), start));
    }
    close(sock);
    pthread_exit(NULL);
}

void *udp_server_thread(void *arg) {
    (void)arg; // sacar despues esto
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serv, cli;
    socklen_t len = sizeof(cli);
    char buffer[512];

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
            pthread_mutex_lock(&resultados_mutex);
            struct ResultadoEntry *r = resultados;
            while (r && r->result.id_measurement != id) r = r->next;
            pthread_mutex_unlock(&resultados_mutex);
            if (r) {
                printf("[✓] ID encontrado. Enviando resultados\n");
                char response[10240];
                int resp_size = packResultPayload(r->result, response, sizeof(response));
                sendto(sock, response, resp_size, 0, (struct sockaddr*)&cli, len);
            }
            else {
                printf("[!] ID 0x%x no encontrado\n", id);
            }
        }
    }
    return NULL;
}
