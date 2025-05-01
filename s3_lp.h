#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef struct log_s {
  char bucket_owner[64];
  char bucket_name[64]; // Only lowercase letters, numbers, dots, and hyphens
  char remote_ip[64];
  char requester_id[64];
  char request_id[64];
  char operation[64];
  char key[1024];
  char request_uri[8192];
  int http_status;
  char err_code[32];
  ssize_t bytes_sent;
  ssize_t object_size;
  time_t ms_ttime;
  time_t ms_tatime;
  char referer[64];
  char user_agent[64];
  char ver_id[64];
  char host_id[64];
  char auth_sig[16];
  char cipher_suite[64];
  char auth_type[64];
  char host_header[64];
  char TLS_ver[8];
  struct tm time; // strftime format: [%d/%b/%Y:%H:%M:%S %z]
} log_t;
