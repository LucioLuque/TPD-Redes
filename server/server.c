
#include "server_funcs.h"

int main() {
    // Configuración del servidor
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

            struct thread_arg_t *arg = malloc(sizeof(*arg));
            arg->client_sock = client_sock;
            arg->client_addr = client;
            pthread_create(&tid, NULL, handle_upload_conn, arg);
        }
    }
    
    return 0;
}
