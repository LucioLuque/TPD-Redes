#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>  /* htonl */
#include <string.h>     /* memcpy */
#include <ctype.h>     /* needed for isprint() */
#include <errno.h>

#include "handle_result.h"

#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 16
#endif 

void hexdump(void *mem, unsigned int len);

// #define NUM_CONN 10

#define E_MINIMUM_DATA    -1
#define E_NOT_ENOUGH_DATA -2
#define E_NO_NEW_LINE     -3
#define E_LINE_TOO_LONG   -4
#define E_INV_LINE_FORMAT -5
#define E_NUMBER_PARSE    -6

struct BW_result *create_bw_result(int num_conn) {
  struct BW_result *result = malloc(sizeof(struct BW_result));
  if (!result) return NULL;
  result->num_conn = num_conn;
  result->conn_bytes = malloc(sizeof(uint64_t) * num_conn);
  result->conn_duration = malloc(sizeof(double) * num_conn);
  if (!result->conn_bytes || !result->conn_duration) {
      free(result->conn_bytes);
      free(result->conn_duration);
      free(result);
      return NULL;
  }
  return result;
}

void free_bw_result(struct BW_result *result) {
  if (!result) return;
  free(result->conn_bytes);
  free(result->conn_duration);
  free(result);
}


void printBwResult(struct BW_result bw_result) {
  printf("id 0x%x\n", bw_result.id_measurement);
  for (int i=0; i< bw_result.num_conn; i++) {
    if (bw_result.conn_duration[i] <= 0) {
      /* No data for this connection */
      continue;
    }
    printf("conn %2d  %20ld bytes %.3f seconds\n", i, bw_result.conn_bytes[i], bw_result.conn_duration[i]);
  }
}

int packResultPayload(struct BW_result bw_result, void *buffer, int buffer_size) {

  int bytes_temp_buffer = 0;

  // bytes   [20 chars] 1.8e+19
  // in-line separator[1] ","
  // duration[6 chars]  20.001 
  // line separator[1] "\n"
  // size: NUM_CONN*(20+1+6+1) + 4  
  //    if NUM_CONN = 10:   284
  // Large enough temporary buffer
  int bytes_needed = (20 + 1 + 6 + 1) * bw_result.num_conn + 4;
  char *my_buffer = malloc(bytes_needed);
  if (!my_buffer) {
    return -1; /* Memory allocation failed */
  }
  memset(my_buffer, 0, bytes_needed);
  
  // Pack id
  uint32_t netorder_id = htonl(bw_result.id_measurement);
  memcpy(my_buffer, &netorder_id, sizeof(netorder_id));
  bytes_temp_buffer += sizeof(netorder_id);

  // Pack each connection measurement
  for (int i=0; i< bw_result.num_conn; i++) {
    char aux[28];
    int  n_aux;
    memset(aux, 0 , sizeof(aux));
    n_aux = sprintf(aux, "%lu,%.3f\n", bw_result.conn_bytes[i], bw_result.conn_duration[i]);
    memcpy(my_buffer+bytes_temp_buffer, aux, n_aux);
    bytes_temp_buffer += n_aux;
  }
  
  if (buffer_size >= bytes_temp_buffer) {
    /* Copy results to buffer given */
    memcpy(buffer, my_buffer, bytes_temp_buffer);
  }
  else {
    /* Not enough buffer space */
    free(my_buffer);
    return -1;
  }
  free(my_buffer);
  return bytes_temp_buffer;
}

int unpackResultPayload(struct BW_result **bw_result, void *buffer, int buffer_size, int num_conn) {
  struct BW_result *result = create_bw_result(num_conn);

  if (!result) {
    return -1; /*fallo memoria alocada*/
  }



  if (buffer_size < (int)sizeof(uint32_t)) {
    /* Buffer too small for minimum data */
    free_bw_result(result);
    return E_MINIMUM_DATA; 
  }


  char *buf = (char *)buffer;
  int offset = 0;
    
  // Unpack id_measurement
  uint32_t netorder_id;
  memcpy(&netorder_id, buf + offset, sizeof(netorder_id));
  result->id_measurement = ntohl(netorder_id);
  offset += sizeof(netorder_id);

  // Unpack each connection measurement
  for (int i = 0; i < num_conn; i++) {
    if (offset >= buffer_size) {
      /* Not enough data */
      free_bw_result(result);
      return E_NOT_ENOUGH_DATA; 
    }
    
    /* Find the end of current line (newline character) */
    int line_start = offset;
    int line_end = line_start;
    while (line_end < buffer_size && buf[line_end] != '\n') {
        line_end++;
    }
    if (line_end >= buffer_size) {
      /* No newline found or buffer overflow */
      free_bw_result(result);
      return E_NO_NEW_LINE; 
    }
    
    // Extract the line (without newline)
    int line_length = line_end - line_start;
    char line[40]; /*  Should be enough for the format  20 + 1+ 6  */
    if (line_length >= (int)sizeof(line)) {
      /* Line too long */
      free_bw_result(result);
      return E_LINE_TOO_LONG; 
    }
    memcpy(line, buf + line_start, line_length);
    line[line_length] = '\0'; /* Null terminate C-string */
    printf("Line [%d,%d] %s\n", line_start, line_end, line);
    
    // Parse the line: "conn_bytes,conn_duration"
    char *comma_pos = strchr(line, ',');
    if (comma_pos == NULL) {
      /* Invalid format - no comma found */
      free_bw_result(result);
      return E_INV_LINE_FORMAT; 
    }
    
    // Split at comma
    *comma_pos = '\0';
    char *bytes_str = line;
    char *duration_str = comma_pos + 1;
    
    /* Parse bytes (uint64_t) */
    char *endptr;
    errno = 0;
    unsigned long long parsed_bytes = strtoull(bytes_str, &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
      free_bw_result(result);
      return E_NUMBER_PARSE;
    }
    result->conn_bytes[i] = (uint64_t)parsed_bytes;
    
    /* Parse duration (double) */
    errno = 0;
    double parsed_duration = strtod(duration_str, &endptr);
    if (errno != 0 || *endptr != '\0') {
      free_bw_result(result);
      return E_NUMBER_PARSE;
    }
    result->conn_duration[i] = parsed_duration;
    
    /* Move to next line (skip the newline) */
    offset = line_end + 1;
  }
  *bw_result = result;
  
  return offset; // Return total bytes consumed
}



// http://grapsus.net/blog/post/Hexadecimal-dump-in-C
void hexdump(void *mem, unsigned int len)
{
  unsigned int i, j;

  for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
  {
    /* print offset */
    if(i % HEXDUMP_COLS == 0)
    {
      printf("0x%06x: ", i);
    }

    /* print hex data */
    if(i < len)
    {
      printf("%02x ", 0xFF & ((char*)mem)[i]);
    }
    else /* end of block, just aligning for ASCII dump */
    {
      printf("   ");
    }

    /* print ASCII dump */
    if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
    {
      for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
      {
        if(j >= len) /* end of block, not really printing */
        {
          putchar(' ');
        }
        else if(isprint(((char*)mem)[j])) /* printable char */
        {
          putchar(0xFF & ((char*)mem)[j]);
        }
        else /* other char */
        {
          putchar('.');
        }
      }
      putchar('\n');
    }
  }
}
