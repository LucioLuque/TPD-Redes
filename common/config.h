#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>
#define NUM_CONN_MAX 10
#define RESULT_BUFFER_SIZE  (sizeof(uint32_t) + \
                             NUM_CONN_MAX*(20+1+6+1))


#define DATA_BUFFER_SIZE 4096 //
#define T 20  // segundos
#define PORT_DOWNLOAD 20251
#define PORT_UPLOAD 20252

#endif // CONFIG_H