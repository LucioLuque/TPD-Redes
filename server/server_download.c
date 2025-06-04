// // server_download.c
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <errno.h>
// #include <signal.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <time.h>

// #define SERVER_PORT 20251
// #define BACKLOG 100
// #define BUFFER_SIZE 1024
// #define DURATION 20        // duración de prueba (segundos)
// #define TIMEOUT 23         // T + 3 segundos

// volatile sig_atomic_t keep_running = 1;

// void handle_sigint(int sig) {
//     keep_running = 0;
// }

// void handle_client(int client_fd) {
//     char buffer[BUFFER_SIZE];
//     memset(buffer, 'A', BUFFER_SIZE);

//     time_t start_time = time(NULL);
//     time_t end_time = start_time + DURATION;

//     while (time(NULL) < end_time) {
//         ssize_t sent = send(client_fd, buffer, BUFFER_SIZE, 0);
//         if (sent <= 0) {
//             perror("Error sending data");
//             break;
//         }
//     }

//     // Espera extra hasta T+3 segundos si cliente no cerró
//     struct timeval timeout;
//     timeout.tv_sec = TIMEOUT - DURATION;
//     timeout.tv_usec = 0;
//     setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

//     char tmp[1];
//     recv(client_fd, tmp, 1, 0);  // Dummy read para ver si el cliente cierra
//     close(client_fd);
//     exit(0);
// }

// int main() {
//     signal(SIGINT, handle_sigint);
//     signal(SIGCHLD, SIG_IGN);  // Evita zombis

//     int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (server_fd < 0) {
//         perror("socket");
//         exit(EXIT_FAILURE);
//     }

//     struct sockaddr_in server_addr;
//     socklen_t addr_len = sizeof(server_addr);

//     memset(&server_addr, 0, addr_len);
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(SERVER_PORT);
//     server_addr.sin_addr.s_addr = INADDR_ANY;

//     int opt = 1;
//     setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

//     if (bind(server_fd, (struct sockaddr*)&server_addr, addr_len) < 0) {
//         perror("bind");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     if (listen(server_fd, BACKLOG) < 0) {
//         perror("listen");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     printf("Server listening on port %d...\n", SERVER_PORT);

//     while (keep_running) {
//         struct sockaddr_in client_addr;
//         socklen_t client_len = sizeof(client_addr);

//         int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
//         if (client_fd < 0) {
//             if (errno == EINTR) continue;
//             perror("accept");
//             continue;
//         }

//         printf("Connection from %s:%d\n", inet_ntoa(client_addr.sin_addr),
//                ntohs(client_addr.sin_port));

//         pid_t pid = fork();
//         if (pid == 0) {
//             // child process
//             close(server_fd);
//             handle_client(client_fd);
//         } else if (pid > 0) {
//             // parent
//             close(client_fd);
//         } else {
//             perror("fork");
//             close(client_fd);
//         }
//     }

//     close(server_fd);
//     printf("Server shutting down.\n");
//     return 0;
// }
