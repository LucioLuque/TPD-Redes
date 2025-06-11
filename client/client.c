#include "client_funcs.h"


int main(int argc, char *argv[]) {
    const char *ip_servidor = NULL;
    bool idle = true;
    bool test_download = true;
    bool test_upload = true;
    int num_conn = NUM_CONN_MAX; // Número de conexiones por defecto
    
    srandom((unsigned)time(NULL) ^ (unsigned)getpid());

    if (parseo(argc, argv, &ip_servidor, &idle, &test_download, &test_upload, &num_conn) != 0) {    
        return 1; // Error en el parseo
    }
    //imprimir todos los campos de parseo
    printf("IP del servidor: %s\n", ip_servidor);
    printf("Test de RTT: %s\n", idle ? "Habilitado" : "Deshabilitado");
    printf("Test de descarga: %s\n", test_download ? "Habilitado" : "Deshabilitado");
    printf("Test de subida: %s\n", test_upload ? "Habilitado" : "Deshabilitado");
    printf("Número de conexiones: %d\n", num_conn);
    printf("[+] Conectando al servidor %s\n", ip_servidor);

    double rtt_idle = 0.0, rtt_down = 0.0, rtt_up = 0.0;
    double bw_download_bps = 0.0, bw_upload_bps = 0.0;
    char src_ip[INET_ADDRSTRLEN] = "0.0.0.0";
    uint32_t id = 0;

    // Paso 1: medir latencia antes de todo (fase idle)
    if (idle) {
        rtt_idle = medir_rtt_promedio_con_tres_intentos(ip_servidor, "idle");
        if (rtt_idle < 0) {
            fprintf(stderr, "[X] Abortando.\n");
            return 1; // Error en la medición de RTT
        }
    } 
    if (test_download) {
        // Paso 2: download test
        rtt_down = medir_rtt_promedio_con_tres_intentos(ip_servidor, "download");
        if (rtt_down < 0) {
            fprintf(stderr, "[X] Abortando.\n");
            return 1; // Error en la medición de RTT
        }
        bw_download_bps = download_test(ip_servidor, src_ip, num_conn);
    }
    
    if (test_upload) {
        // Paso 3: upload test
        rtt_up = medir_rtt_promedio_con_tres_intentos(ip_servidor, "upload");
        if (rtt_up < 0) {
            fprintf(stderr, "[X] Abortando.\n");
            return 1; // Error en la medición de RTT
        }
        id = generate_id();
        upload_test(ip_servidor, id, num_conn);

        // Paso 4: consultar resultados
        struct BW_result resultado;
        sleep(1); // Esperar un segundo antes de consultar resultados xq 
                // el upload puede tardar un poco en completarse
        consultar_resultados(ip_servidor, PORT_DOWNLOAD, id, &resultado, num_conn);

        // Calcular avg upload bps
        double total = 0;
        for (int i = 0; i < num_conn; i++) {
            if (resultado.conn_duration[i] > 0)
                total += (resultado.conn_bytes[i] * 8.0) / resultado.conn_duration[i];
        }
        bw_upload_bps = total / num_conn;
    }

    

    // Paso 5: exportar el JSON, por ahora manda por prints
    exportar_json(bw_download_bps, bw_upload_bps, rtt_idle, rtt_down, rtt_up, src_ip, ip_servidor, num_conn);
    return 0;
}
