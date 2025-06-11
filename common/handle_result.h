#ifndef HANDLE_RESULT_H
#define HANDLE_RESULT_H

#include <stdint.h>
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>  /* htonl */
#include <string.h>     /* memcpy */
#include <ctype.h>     /* needed for isprint() */
#include <errno.h>
#include "config.h"

#define HEXDUMP_COLS 16
#define E_MINIMUM_DATA    -1
#define E_NOT_ENOUGH_DATA -2
#define E_NO_NEW_LINE     -3
#define E_LINE_TOO_LONG   -4
#define E_INV_LINE_FORMAT -5
#define E_NUMBER_PARSE    -6

struct BW_result {
  uint32_t id_measurement;
  uint64_t conn_bytes[NUM_CONN_MAX];
  double conn_duration[NUM_CONN_MAX];
};

void printBwResult(struct BW_result bw_result);
int packResultPayload(struct BW_result bw_result, void *buffer, int buffer_size);
int unpackResultPayload(struct BW_result *bw_result, void *buffer, int buffer_size);
void hexdump(void *mem, unsigned int len);
#endif // HANDLE_RESULT_H