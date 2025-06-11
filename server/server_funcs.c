
#include "server_funcs.h"
struct ResultadoEntry *resultados = NULL;

pthread_mutex_t resultados_mutex = PTHREAD_MUTEX_INITIALIZER;
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

void *handle_download_conn(void *arg) {
    struct thread_arg_t *args = (struct thread_arg_t *)arg;
    int sock = args->client_sock;
    struct sockaddr_in client_addr = args->client_addr;
    free(arg);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("[+] Nueva conexion de descarga desde %s\n", client_ip);

    char buffer[DATA_BUFFER_SIZE];
    memset(buffer, 'D', DATA_BUFFER_SIZE);

    struct timeval start, now;
    gettimeofday(&start, NULL);
    double elapsed = 0.0;

    while (elapsed < T + 3) {
        ssize_t sent = send(sock, buffer, DATA_BUFFER_SIZE, 0);
        if (sent <= 0) break;  // error o cliente cerro conexion

        gettimeofday(&now, NULL);
        elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1e6;

        // Si pasaron T segundos y no se envio nada, se cierra
        // if (elapsed >= T) break;
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
    (void)arg; // sacar despues esto
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