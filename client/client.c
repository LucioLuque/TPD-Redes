#include "client_funcs.h"

int main(int argc, char *argv[]) {
    const char *ip_servidor = NULL;
    bool idle = true;
    bool test_download = true;
    bool test_upload = true;
    bool json = true;
    int num_conn = NUM_CONN_MAX; // Número de conexiones maximas por defecto
    const char *ip_hostremoto = NULL;
    int json_port = 9999; // por defecto usa el 9999, sino se asigna el valor del argumento
    
    srandom((unsigned)time(NULL) ^ (unsigned)getpid());

    if (parse_arguments(argc, argv, &ip_servidor, &num_conn, &idle, &test_download, &test_upload, &json, &ip_hostremoto, &json_port) != 0) {   
        return 1; // Error en el parseo
    }
    //imprimir todos los campos de parseo
    printf("IP del servidor: %s\n", ip_servidor);
    printf("Número de conexiones: %d\n", num_conn);
    printf("Test de RTT: %s\n", idle ? "Habilitado" : "Deshabilitado");
    printf("Test de descarga: %s\n", test_download ? "Habilitado" : "Deshabilitado");
    printf("Test de subida: %s\n", test_upload ? "Habilitado" : "Deshabilitado");
    printf("Exportar JSON: %s\n", json ? "Habilitado" : "Deshabilitado");
    printf("IP del host remoto: %s\n", ip_hostremoto);
    printf("Puerto JSON: %d\n", json_port);

    printf("[+] Conectando al servidor %s\n", ip_servidor);

    double rtt_idle = 0.0, rtt_down = 0.0, rtt_up = 0.0;
    double bw_download_bps = 0.0, bw_upload_bps = 0.0;
    char src_ip[INET_ADDRSTRLEN] = "0.0.0.0";
    uint32_t id = 0;
    
    if (idle) {
        // Paso 1: medir latencia (fase idle)
        rtt_idle = rtt_test(ip_servidor, "idle");
        if (rtt_idle < 0) {
            fprintf(stderr, "[X] Abortando.\n");
            return 1; // Error en la medición de RTT
        }
    } 

    if (test_download) {
        // Paso 2: download test
        bw_download_bps = download_test(ip_servidor, src_ip, num_conn, &rtt_down);
    }
    
    if (test_upload) {
        // Paso 3: upload test
        id = generate_id();
        upload_test(ip_servidor, id, num_conn, &rtt_up);    

        // Paso 4: consultar resultados
        struct BW_result resultado;
        sleep(1); // Esperar un segundo antes de consultar resultados xq el upload puede tardar un poco en completarse

        query_results_from_server(ip_servidor, PORT_DOWNLOAD, id, &resultado, num_conn);

        // Calcular avg upload bps
        double total = 0;
        for (int i = 0; i < num_conn; i++) {
            if (resultado.conn_duration[i] > 0)
                total += (resultado.conn_bytes[i] * 8.0) / resultado.conn_duration[i];
        }
        bw_upload_bps = total / num_conn;
    }
    
    if (json){
        // Paso 5: exportar el JSON
        export_json(bw_download_bps, bw_upload_bps, rtt_idle, rtt_down, rtt_up, src_ip, ip_servidor, num_conn, ip_hostremoto, json_port);
    }
    
    return 0;
}
