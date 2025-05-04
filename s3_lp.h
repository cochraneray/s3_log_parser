#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DJB2HASH 5381
#define IP_HASH 2000003
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
	struct tm time;	   // strftime format: [%d/%b/%Y:%H:%M:%S %z]

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
	uint32_t timestamp;			// mktime
	uint32_t ip_hash;			// remote_ip
	uint32_t podcast_hash;		// key
	uint32_t key_hash;			// key
	uint16_t bytes_sent_kb;		// bytes_sent / 1024
	uint16_t object_size_kb;	// object_size
	uint16_t download_time_ms;	// ms_ttime
	uint8_t status_code;		// http_code
	uint8_t platform_id;		// user agent
	uint8_t device_type;		// user agent
	uint8_t country_id;			// remote_ip
	uint8_t completion_percent; // bytes_sent / object_size
	uint8_t flags;				// 8 Bit Flag for checking download progress
} s_log_t;

// Unique IP Address manager, uses Hash Table
typedef struct ip_track_s {
	uint32_t ip_hashes[IP_HASH];
	size_t count;
} ip_track_t;

void process_log(FILE *log, FILE *output);

void parse_log_entry(char *in_log, p_log_t *full_log);
inline int fast_atoi(const char *str);
inline int64_t fast_atol(const char *str);

// Extract Log and Send to Slim
void extract_log_entry(p_log_t *full_log, s_log_t *slim_log, ip_track_t *ip_addrs);
uint32_t hash_podcast(const char *key);
uint32_t hash_key(const char *key);
uint8_t extract_platform(const char *user_agent);
uint8_t extract_device(const char *device);
uint8_t extract_location(const char *location);
uint8_t set_flags(p_log_t *full_log, s_log_t *slim_log, ip_track_t *ip_addrs);
int is_unique_ip(ip_track_t *ip_addrs, uint32_t ip_hash, uint32_t key_hash);

// Process Slim Logs
void process_slim_logs(s_log_t *slim_log, int num_entrys, FILE *output);
