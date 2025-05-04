#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define PASS_HASH 2000003
#define LOG_SIZE 8192
#define OPTIONS "f:h"
#define BATCH_SIZE 10000

#define PARSE_TIME_FAIL 3

typedef struct p_log_s {
  char buffer[LOG_SIZE];
  size_t length;

  char *bucket_owner;
  char *bucket_name; // Only lowercase letters, numbers, dots, and hyphens
  struct tm time;    // strftime format: [%d/%b/%Y:%H:%M:%S %z]

  char *remote_ip;
  char *requester_id;
  char *request_id;

  char *operation;
  char *key;

  char *request_uri;
  int http_code;
  char *err_code;

  ssize_t bytes_sent;
  ssize_t object_size;
  time_t ms_ttime;
  time_t ms_tatime;

  char *referer;
  char *user_agent;
  char *ver_id;
  char *host_id;

  char *auth_sig;
  char *cipher_suite;
  char *auth_type;
  char *host_header;
  char *TLS_ver;
  char *ARN_ap;
} p_log_t;

// Slimed down log struct - POST-PROCESSING
// 25 bytes
typedef struct s_log_s {
  uint32_t timestamp;
  uint32_t ip_hash;
  uint16_t episode_id;
  uint16_t podcast_id;
  uint16_t bytes_sent_kb;
  uint16_t object_size_kb;
  uint16_t download_time_ms;
  uint8_t status_code;
  uint8_t platform_id;
  uint8_t device_type;
  uint8_t country_id;
  uint8_t completion_percent;
  uint8_t flags;
} s_log_t;

void process_log(FILE *log, FILE *output);
void parse_log_entry(char *in_log, p_log_t *full_log);
void extract_log_entry(p_log_t *full_log, s_log_t *slim_log);
void process_slim_logs(s_log_t *slim_log, int num_entrys, FILE *output);
