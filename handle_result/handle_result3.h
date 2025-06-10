#ifndef HANDLE_RESULT_H
#define HANDLE_RESULT_H

#include <stdint.h>

// 

struct BW_result {
  uint32_t id_measurement;
  uint64_t *conn_bytes;
  double *conn_duration;
  int num_conn;
};

struct BW_result *create_bw_result(int num_conn);
void free_bw_result(struct BW_result *result);
void printBwResult(struct BW_result bw_result);
int packResultPayload(struct BW_result bw_result, void *buffer, int buffer_size);
int unpackResultPayload(struct BW_result **bw_result, void *buffer, int buffer_size, int num_conn);

#endif