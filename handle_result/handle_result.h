#ifndef HANDLE_RESULT_H
#define HANDLE_RESULT_H

#include <stdint.h>

// estas variables deberian ir en otro archivo comun
//  server, client  y hanlde_result porq aca muhco que no va
#define NUM_CONN_MAX 10
#define RESULT_BUFFER_SIZE  (sizeof(uint32_t) + \
                             NUM_CONN_MAX*(20+1+6+1))


#define DATA_BUFFER_SIZE 4096 //
#define T 20  // segundos

struct BW_result {
  uint32_t id_measurement;
  uint64_t conn_bytes[NUM_CONN_MAX];
  double conn_duration[NUM_CONN_MAX];
};

void printBwResult(struct BW_result bw_result);
int packResultPayload(struct BW_result bw_result, void *buffer, int buffer_size);
int unpackResultPayload(struct BW_result *bw_result, void *buffer, int buffer_size);

#endif