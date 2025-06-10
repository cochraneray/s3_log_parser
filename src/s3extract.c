#include "../include/s3extract.h"
#include <stdint.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	char *input_file = NULL;
	char *output_file = NULL;
	FILE *ifp = stdin;
	FILE *ofp = stdout;

	int group_by = GROUP_NONE;
	int verbose = 0;
	int err = 0;

	{
		int opt = 0;
		while ((opt = getopt(argc, argv, EXTRACT_OPTIONS)) != -1) {
			switch (opt) {
			case 'f': {
				if (optarg == NULL) {
					ifp = stdin;
				}
				else {
					input_file = optarg;
				}
				break;
			}
			case 'o': {
				if (optarg == NULL) {
					ofp = stdout;
				}
				else {
					output_file = optarg;
				}
				break;
			}
			case 'g': {
				if (optarg == NULL) {
					fprintf(stderr, "Invalid Grouping! Use: p(odcast), i(p), t(ime), or n(one)\n");
					exit(EXIT_FAILURE);
				}
				switch (*optarg) {
				case 'p':
					group_by = GROUP_PODCAST;
					break;
				case 'i':
					group_by = GROUP_IP;
					break;
				case 't':
					group_by = GROUP_TIME;
					break;
				case 'n':
					group_by = GROUP_NONE;
					break;
				default:
					fprintf(stderr, "Invalid Group. Use: p(odcast), i(p), t(ime), or n(one)\n");
					exit(EXIT_FAILURE);
				}
				break;
			}
			case 'v': {
				verbose = (verbose == 1) ? 0 : 1;
				break;
			}
			case 'h': {
				print_help();
				exit(EXIT_FAILURE);
			}
			default:
				print_help();
				exit(EXIT_FAILURE);
			}
		}
	} // End getopt scope

	// Open Files
	if (input_file) {
		ifp = fopen(input_file, "rb");
		if (!ifp) {
			perror("fopen input_file");
			exit(EXIT_FAILURE);
		}
	}

	if (output_file) {
		ofp = fopen(output_file, "w");
		if (!ofp) {
			perror("fopen output_file");
			exit(EXIT_FAILURE);
		}
	}

	err = extract_to_json(ifp, ofp, group_by, verbose);

	if (err == -1) {
		fprintf(stderr, "Extract to json failed, aborting");
	}

	if (ifp != stdin) {
		fclose(ifp);
	}
	if (ofp != stdout) {
		fclose(ofp);
	}

	exit(EXIT_SUCCESS);
}

int
extract_to_json(FILE *input, FILE *output, int group_by, int verbose_flag)
{
	s_log_t log_entry;
	uint64_t entries = 0;

	if (group_by == GROUP_NONE) {
		fprintf(output, "{\n");
		fprintf(output, "  \"logs\": [\n");

		int first_entry = 1;
		while (fread(&log_entry, sizeof(s_log_t), 1, input) == 1) {
			print_log_as_json(&log_entry, output, first_entry);
			first_entry = 0;
			entries++;

			if (verbose_flag == 1 && entries % FLUSH_THRESHOLD == 0) {
				fprintf(stderr, "Processed: %lu entries\n", entries);
			}
		}

		fprintf(output, "\n  ],\n");
		fprintf(output, "  \"entries\": %lu\n", entries);
		fprintf(output, "}\n");
	}
	else {
		log_group_t *groups = NULL;
		int group_count = 0;
		int group_capacity = GROUP_THRESHOLD;

		groups = calloc(group_capacity, sizeof(log_group_t));
		if (!groups) {
			perror("calloc group");
			return -1;
		}

		//
		while (fread(&log_entry, sizeof(s_log_t), 1, input) == 1) {
			uint32_t group_key;

			switch (group_by) {
			case GROUP_PODCAST:
				group_key = log_entry.podcast_hash;
				break;
			case GROUP_IP:
				group_key = log_entry.ip_hash;
				break;
			case GROUP_TIME:
				group_key = log_entry.timestamp / SECONDS_IN_DAY;
				break;
			default:
				group_key = 0;
				break;
			}

			// Find Group if it exists, otherwise create
			int group_index = -1;
			for (int i = 0; i < group_count; i++) {
				if (groups[i].group_key == group_key) {
					group_index = i;
					break;
				}
			}

			if (group_index == -1) {
				if (group_count >= group_capacity) {
					group_capacity *= 2;
					groups = realloc(groups, group_capacity * sizeof(log_group_t));
					if (!groups) {
						perror("realloc groups");
						return -1;
					}
				}

				group_index = group_count++;
				groups[group_index].group_key = group_key;
				groups[group_index].logs = calloc(64, sizeof(s_log_t));
				groups[group_index].count = 0;
				groups[group_index].capacity = 64;
			}

			log_group_t *group = &groups[group_index];
			if (group->count >= group->capacity) {
				group->capacity *= 2;
				group->logs = realloc(group->logs, group->capacity * sizeof(s_log_t));
				if (!group->logs) {
					perror("calloc group logs:");
					return -1;
				}
			}

			group->logs[group->count++] = log_entry;
			entries++;

			if (verbose_flag == 1 && entries % FLUSH_THRESHOLD == 0) {
				fprintf(stderr, "Processed: %lu entries, %d groups\n", entries, group_count);
			}
		}

		print_grouped_json(groups, group_count, output, group_by);

		for (int i = 0; i < group_count; i++) {
			free(groups[i].logs);
		}
		free(groups);
	}
	if (verbose_flag) {
		fprintf(stderr, "Total entries processed: %lu\n", entries);
	}
	return 0;
}

void
print_log_as_json(s_log_t *log, FILE *output, int is_first)
{
	if (!is_first) {
		fprintf(output, ",\n");
	}

	char *time_str = format_timestamp(log->timestamp);

	fprintf(output, "    {\n");
	fprintf(output, "      \"timestamp\": %u, \n", log->timestamp);
	fprintf(output, "       \"time\": \"%s\", \n", time_str);
	fprintf(output, "       \"ip_hash\": \"%s\", \n", format_hash(log->ip_hash));
	fprintf(output, "       \"podcast_hash\": \"%s\", \n", format_hash(log->podcast_hash));
	fprintf(output, "       \"key_hash\": \"%s\", \n", format_hash(log->key_hash));
	fprintf(output, "       \"bytes_sent_kb\": %u, \n", log->bytes_sent_kb);
	fprintf(output, "       \"object_size_kb\": %u, \n", log->object_size_kb);
	fprintf(output, "       \"download_time_ms: %u, \n", log->download_time_ms);
	fprintf(output, "       \"http_code\": %u, \n", log->http_code);
	fprintf(output, "       \"system_id\": %u, \n", log->system_id);
	fprintf(output, "       \"platform_id\": %u, \n", log->platform_id);
	fprintf(output, "       \"completion_percent\": %u, \n", log->completion_percent);
	fprintf(output, "       \"flags\": %u, \n", log->flags);
	fprintf(output, "    }");

	free(time_str);
}

void
print_grouped_json(log_group_t *groups, int group_count, FILE *output, int group_by)
{
	fprintf(output, "{\n");
	fprintf(output, "  \"grouped_by\": \"%s\", \n", get_group_name(group_by));
	fprintf(output, "  \"groups\":  {\n");

	for (int i = 0; i < group_count; i++) {
		if (i > 0) {
			fprintf(output, ",\n");
		}

		char *group_key_str;
		if (group_by == GROUP_TIME) {
			time_t timestampd = groups[i].group_key * SECONDS_IN_DAY;
			group_key_str = format_timestamp(timestampd);
		}
		else {
			group_key_str = format_hash(groups[i].group_key);
		}

		fprintf(output, "    \"%s\": {\n", group_key_str);
		fprintf(output, "      \"count\": %d, \n", groups[i].count);
		fprintf(output, "      \"logs\": [\n");


		for (int j = 0; j < groups[i].count; j++) {
			// j == 0 provides bool for is_first variable
			print_log_as_json(&groups[i].logs[j], output, j == 0);
		}

		fprintf(output, "\n      ]\n");
		fprintf(output, "    }");

		free(group_key_str);
	}

	fprintf(output, "\n  },\n");
	fprintf(output, "  \"total_groups\": %d\n", group_count);
	fprintf(output, "}\n");
}

char *
get_group_name(int group_by)
{
	switch (group_by) {
	case GROUP_PODCAST:
		return "podcast";
	case GROUP_IP:
		return "ip_address";
	case GROUP_TIME:
		return "day";
	default:
		return "none";
	}
}

char *
format_timestamp(uint32_t timestamp)
{
	char *time_str = malloc(32);
	if (!time_str) {
		return NULL;
	}

	time_t t = (time_t)timestamp;
	struct tm *time_info = localtime(&t);
	if (time_info == NULL) {
		snprintf(time_str, 32, "INVALID_TIME_%u", timestamp);
		return time_str;
	}

	if (strftime(time_str, 32, "%Y-%m-%d %H:%M:%S", time_info) == 0) {
		snprintf(time_str, 32, "FORMAT_ERROR_%u", timestamp);
	}
	return time_str;
}

char *
format_hash(uint32_t hash)
{
	char *hash_str = malloc(9);
	snprintf(hash_str, 9, "%08x", hash);
	return hash_str;
}

void
print_help(void)
{
	printf("S3 Log Extraction Tool - Simple JSON Export\n");
	printf("Usage: ./s3_extract [options]\n\n");
	printf("Options:\n");
	printf("    -f <file>      binary log file (default: stdin)\n");
	printf("    -o <file>      Output JSON file (default: stdout\n");
	printf("    -g             Group by: p(odcast), i(p), t(ime/day), n(one) [default: none]\n");
	printf("    -v             Verbose Output, multiple inputs will toggle the flag\n");
	printf("    -h             This page right here\n\n");
	printf("Example usage:\n");
	printf("\t./s3_extract -f logs.bin -o output.json          // Simple Output\n");
	printf("\t./s3_extract -f logs.bin -o output.json -g p     // Podcast Groping\n");
	printf("\t./s3_extract -f logs.bin -o output.json -g i     // IP Hash Grouping\n");
	printf("\t./s3_extract -f logs.bin -o output.json -g t     // Time Grouping\n");
}
