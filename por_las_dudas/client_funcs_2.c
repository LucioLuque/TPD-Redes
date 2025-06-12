
// double download_test(const char *server_ip, char *src_ip, int num_conn, double *rtt_download) {
//     int pipes[num_conn][2];
//     pid_t pids[num_conn];
//     struct sockaddr_in server;
//     socklen_t addr_len = sizeof(server);
    
//     int pipe_rtt[2];
//     if (pipe(pipe_rtt)<0) {perror("pipe RTT"); exit(1); }
    
//     pid_t pid_rtt = paralel_rtt_test(server_ip, "download", pipe_rtt);

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
//     // Espera a que el hijo de RTT termine
//     waitpid(pid_rtt, NULL, 0);

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
//     double rtt_avg = 0.0;
//     read(pipe_rtt[0], &rtt_avg, sizeof(rtt_avg));
//     close(pipe_rtt[0]);
//     *rtt_download = rtt_avg;

//     return (double)total * 8.0 / elapsed;  // bps reales
// }

// double download_test(const char *server_ip, char *src_ip, int num_conn, double *rtt_download) {
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


//server funcs
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


// struct BW_result *get_or_create_result(uint32_t id_measurement, 
//                                        const char *client_ip) {
//     pthread_mutex_lock(&results_mutex);
//     struct ResultEntry *actual = results;
//     for 
//     while (actual) {
//         if (actual->result.id_measurement == id_measurement) {
//             pthread_mutex_unlock(&results_mutex);
//             return &actual->result;
//         }
//         actual = actual->next;
//     }
//     struct ResultEntry *new = malloc(sizeof(struct ResultEntry));
//     new->result.id_measurement = id_measurement;
//     memset(new->result.conn_bytes, 0, sizeof(new->result.conn_bytes));
//     memset(new->result.conn_duration, 0, sizeof(new->result.conn_duration));
//     new->next = results;
//     results = new;
//     pthread_mutex_unlock(&results_mutex);
//     return &new->result;
// }