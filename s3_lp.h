#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DJB2HASH 5381 // djb2 prime
#define IP_HASH 12289 // nice prime number
#define LOG_SIZE 8192
#define OPTIONS "f:o:h"
#define BATCH_SIZE 10000
#define MEGABYTE 1048576

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
  char *acl_required;
  char *range_get;
  size_t byte_start;
  size_t byte_end;
} p_log_t;

// Slimed down log struct - POST-PROCESSING
// 25 bytes
typedef struct s_log_s {
  uint32_t timestamp;         // mktime
  uint32_t ip_hash;           // remote_ip
  uint32_t podcast_hash;      // key
  uint32_t key_hash;          // key
  uint16_t bytes_sent_kb;     // bytes_sent / 1024
  uint16_t object_size_kb;    // object_size
  uint16_t download_time_ms;  // ms_ttime
  uint8_t status_code;        // http_code
  uint8_t system_id;          // user agent
  uint8_t platform_id;        // user agent
  uint8_t country_id;         // remote_ip
  uint8_t completion_percent; // bytes_sent / object_size
  uint8_t flags;              // 8 Bit Flag for checking download progress
} s_log_t;

// Enum Codes for System ID and Platform ID
typedef enum {
  UNKNOWN = 0,
  BLUBRRY = 1,
  SPOTIFY = 2,
  APPLE_PODCASTS = 3,
  GOOGLE_PODCASTS = 4,
  OVERCAST = 5,
  POCKET_CASTS = 6,
  CASTRO = 7,
  STITCHER = 8,
  AMAZON_MUSIC = 9,
  PODCAST_ADDICT = 10,
  YOUTUBE = 11,
  RSS_DIRECT = 12,
  SOUNDCLOUD = 13,
  CASTBOX = 14,
  PLAYER_FM = 15,
  WEB_PLAYER = 16
} system_id_t;

// bit value flags for device and OS
typedef enum {
  DEV_UNKNOWN = 0,
  DEV_DESKTOP = 1,
  DEV_MOBILE = 2,
  DEV_TABLET = 3,
  DEV_SMART_SPEAKER = 4,
  DEV_AUTOMOTIVE = 5,
  DEV_TV = 6,
  DEV_WEARABLE = 7,

  // Bit Shift Left 8 Places
  OS_UNKNOWN = 0 << 8,
  OS_ANDROID = 1 << 8,
  OS_IOS = 2 << 8,
  OS_WINDOWS = 3 << 8,
  OS_MACOS = 4 << 8,
  OS_LINUX = 5 << 8,
  OS_CHROMECAST = 6 << 8,
  OS_ROKU = 7 << 8,
  OS_WATCHOS = 8 << 8
} platform_id_t;

// Unique IP Address manager, uses Hash Table
typedef struct ip_track_s {
  uint64_t *ip_hashes;
  size_t count;
  size_t capacity;
} ip_track_t;

void process_log(FILE *log, FILE *output);

void parse_log_entry(char *in_log, p_log_t *full_log);
inline int fast_atoi(const char *str);
inline int64_t fast_atol(const char *str);

// Extract Log and Send to Slim
void extract_log_entry(p_log_t *full_log, s_log_t *slim_log);
uint32_t hash_podcast(const char *key);
uint32_t hash_key(const char *key);
uint8_t extract_system(const char *device);
uint8_t extract_platform(const char *user_agent);
uint8_t extract_location(const char *location);
uint8_t set_flags(p_log_t *full_log, s_log_t *slim_log);
int is_unique_ip(uint32_t ip_hash, uint32_t key_hash);
int check_pattern(const char *check_str, const char *pattern);

// Process Slim Logs
void process_slim_logs(s_log_t *slim_log, int num_entrys, FILE *output);
