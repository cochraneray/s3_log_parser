#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "s3lp.h"

#define EXTRACT_OPTIONS "f:o:g:vh"
#define GROUP_NONE 0
#define GROUP_PODCAST 1
#define GROUP_IP 2
#define GROUP_TIME 3
#define GROUP_THRESHOLD 128
#define FLUSH_THRESHOLD 10000
#define SECONDS_IN_DAY 86400

typedef struct log_group_s {
	uint32_t group_key;
	s_log_t *logs;
	int count;
	int capacity;
} log_group_t;

int extract_to_json(FILE *input, FILE *output, int group_by, int verbose_flag);
void print_log_as_json(s_log_t *log, FILE *output, int is_first);
void print_grouped_json(log_group_t *groups, int group_count, FILE *output, int group_by);
char *get_group_name(int group_by);
char *format_timestamp(uint32_t timestamp);
char *format_hash(uint32_t hash);
void print_help(void);


#ifdef __cplusplus
}
#endif
