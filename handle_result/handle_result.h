#ifndef HANDLE_RESULT_H
#define HANDLE_RESULT_H

#include <stdint.h>

#define NUM_CONN_MAX 100

struct BW_result {
  uint32_t id_measurement;
  uint64_t conn_bytes[NUM_CONN_MAX];
  double conn_duration[NUM_CONN_MAX];
};

void printBwResult(struct BW_result bw_result);
int packResultPayload(struct BW_result bw_result, void *buffer, int buffer_size);
int unpackResultPayload(struct BW_result *bw_result, void *buffer, int buffer_size);

#endif