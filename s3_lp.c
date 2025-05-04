// S3 Log Parser
//
//
//
#include "s3_lp.h"

int
main(int argc, char *argv[])
{
	char *filename = NULL;
	char *output_file = NULL;
	FILE *ifp = stdin; // default file path to stdin
	FILE *ofp = stdout;

	int err_flag = -1;

	{
		int opt = -1;

		while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
			switch (opt) {
			case 'f': {
				if (optarg == NULL) {
					fprintf(stderr, "-f requires a filename to parse, defaults to stdin");
					exit(EXIT_FAILURE);
				}
				filename = optarg;
				break;
			}
			case 'o': {
				if (optarg == NULL) {
					fprintf(stderr, "-o requires an output filename, defaults to stdout");
					exit(EXIT_FAILURE);
				}
				output_file = optarg;
				break;
			}
			case 'h': {
				fprintf(stderr, "USAGE: ./s3_lp -[h] -f <filepath> -o <output_filepath>\n"
								"\t-f filepath: override default filepath from stdin\n"
								"\t-o filepath: override default output filepath from stdout\n"
								"\t-h display options\n");
				exit(EXIT_SUCCESS);
			}
			default:
				fprintf(stderr, "USAGE: ./s3_lp -[options] -f <filepath>\n\tUse -h to "
								"get help:\n\n\t./s3_lp -h");
				exit(EXIT_SUCCESS);
			}
		}
	}

	if (filename != NULL) {
		ifp = fopen(filename, "r");
		if (ifp == NULL) {
			perror("fopen");
			exit(EXIT_FAILURE);
		}
	}

	process_log(ifp, ofp);

	fclose(ifp);
	fclose(ofp);
	exit(EXIT_SUCCESS);
}

// LOG PROCESSING DRIVER -----------------------------------------------------------------------
void
process_log(FILE *log, FILE *output)
{
	char log_entry[LOG_SIZE];
	p_log_t *batch_parsed_logs = malloc(BATCH_SIZE * sizeof(p_log_t));
	s_log_t *batch_slim_logs = malloc(BATCH_SIZE * sizeof(s_log_t));

	int count = 0;

	while (fgets(log_entry, sizeof(log_entry), log)) {
		parse_log_entry(log_entry, &batch_parsed_logs[count]);
		extract_log_entry(&batch_parsed_logs[count], &batch_slim_logs[count]);
		++count;

		if (count >= BATCH_SIZE) {
			process_slim_logs(batch_slim_logs, count, output);
		}
	}

	if (count > 0) {
		process_slim_logs(batch_slim_logs, count, output);
	}

	free(batch_parsed_logs);
	free(batch_slim_logs);
}

// PARSE LOG LINE -> FULL LOG STRUCT--------------------------------------------------------
void
parse_log_entry(char *in_log, p_log_t *full_logs)
{
	char *source = in_log;
	char *dest = full_logs->buffer;
	char *field = full_logs->buffer;
	int field_index = 0;
	int in_quote = 0;
	int err_flags = 0;

	// While source address isnt null (since fgets pulled until newline in process log)
	while (*source) {
		// Quote Checker
		if (*source == '"') {
			in_quote = !in_quote;
			*dest++ = *source++;
			continue;
		}

		// Space Delimiter indicates new field
		if (*source == ' ' && !in_quote) {
			*dest++ = '\0'; // Set Dest address that holds space to \0 and post-increment

			switch (field_index) {
			// Bucket Owner
			// 79a59df900b949e55d96a1e698fbacedfd6e09d98eacf8f8d5218e7cd47ef2be
			case 0:
				full_logs->bucket_owner = field;
				break;

			// Bucket Name
			// amzn-s3-demo-bucket1
			case 1:
				full_logs->bucket_name = field;
				break;

			// Time
			//[06/Feb/2019:00:00:38 +0000]
			case 2: {
				char *close_bracket;
				char *strip_time;
				char time_buffer[32]; // allocate time buffer
				struct tm temp_time = {0};
				// Read from after the first bracket '['
				strncpy(time_buffer, field + 1,
						sizeof(time_buffer)); // copy field into time buffer
											  // Find closing bracket if it exists
				close_bracket = strchr(time_buffer, ']');
				// Check if closing bracket in the buffer
				if (close_bracket != NULL) {
					*close_bracket = '\0';
				}
				// strip time and fille temp_time
				strip_time = strptime(time_buffer, "%d/%b/%Y:%H:%M:%S %z", temp_time);

				// Check for success
				if (strip_time != NULL) {
					full_logs->time = temp_time;
				}
				else {
					// set time struct to 0, error
					memset(&full_logs->time, 0, sizeof(time));
					fprintf(stderr, "Failed to set time for: %s", full_logs->bucket_name);
				}
			} break;

			// Remote IP
			// 192.0.2.3
			case 3:
				full_logs->remote_ip = field;
				break;
				// Requester ID
				// 79a59df900b949e55d96a1e698fbacedfd6e09d98eacf8f8d5218e7cd47ef2be
				// arn:aws:sts::123456789012:assumed-role/roleName/test-role
			case 4:
				full_logs->requester_id = field;
				break;

			// Request ID
			// 3E57427F33A59F07
			case 5:
				full_logs->request_id = field;
				break;

			// Operation
			// REST.PUT.OBJECT
			// REST.GET.OBJECT
			case 6:
				full_logs->operation = field;
				break;

			// Key
			// /photos/2019/08/puppy.jpg
			case 7:
				full_logs->key = field;
				break;

			// Request URI
			// "GET /amzn-s3-demo-bucket1/photos/2019/08/puppy.jpg?x-foo=bar HTTP/1.1"
			case 8:
				full_logs->request_uri = field;
				break;

			// HTTP Status
			// 200
			case 9: {
				int temp = fast_atoi(field);
				if (temp > 599 || temp < 200) { // Should be in appropriate form
					full_logs->http_code = 0;
				}
				else {
					full_logs->http_code = fast_atoi(field);
				}
				break;
			}

			// Error Codes
			// NoSuchBucket
			case 10:
				full_logs->err_code = field;
				break;

			// Bytes Sent
			// 2662992
			case 11: {
				full_logs->bytes_sent = fast_atol(field);
				break;
			}

			// Object Size
			// 3462992
			case 12: {
				full_logs->object_size = fast_atol(field);
				break;
			}
				// Total Time - Number of Miliseconds
			// 70
			case 13: {
				full_logs->ms_ttime = fast_atol(field);
				break;
			}
			// Turn Around Time - Number of Miliseconds
			// 10
			case 14: {
				full_logs->ms_tatime = fast_atoi(field);
				break;
			}
			// Referer
			// "http://www.example.com/webservices"
			case 15:
				full_logs->referer = field;
				break;
			// User Agent
			// "curl/7.15.1"
			case 16:
				full_logs->user_agent = field;
				break;
			// Version ID
			// 3HL4kqtJvjVBH40Nrjfkd
			case 17:
				full_logs->ver_id = field;
				break;
			// Host ID
			// s9lzHYrFp76ZVxRcpX9+5cjAnEH2ROuNkd2BHfIa6UkFVdtjf5mKR3/eTPFvsiP/XV/VLi31234=
			case 18:
				full_logs->host_id = field;
				break;
			// Authentication Signature
			// SigV2
			case 19:
				full_logs->auth_sig = field;
				break;
			// Cipher Suite
			// ECDHE-RSA-AES128-GCM-SHA256
			case 20:
				full_logs->cipher_suite = field;
				break;
			// Authentication Type
			// AuthHeader
			case 21:
				full_logs->auth_type = field;
				break;
			// Host Header
			// s3.us-west-2.amazonaws.com
			case 22:
				full_logs->host_header = field;
				break;
			// TLS Version
			// TLSv1.2
			case 23:
				full_logs->TLS_ver = field;
				break;
			// ARN Access Point
			// arn:aws:s3:us-east-1:123456789012:accesspoint/example-AP
			case 24:
				full_logs->ARN_ap = field;
				break;
			case 25:
				full_logs->acl_required = field;
			default:
				fprintf(stderr, "Error, outside the acceptable bounds of this intake function");
			}
			field_index++; // When field is set we need to increment
			source++;	   // Increment source separately
			field = dest;  // Set field to dest current address (start of next log field)
			continue;
		}


		// Range field for partial downloads
		// https://docs.aws.amazon.com/AmazonCloudFront/latest/DeveloperGuide/RangeGETs.html
		if (*source == ' ' && field_index == 26 && full_logs->http_code == 206 && !in_quote) {
			full_logs->range_get = field;
			sscanf(full_logs->range_get, "\"bytes%lu-%lu\"", &full_logs->byte_start, &full_logs->byte_end);

			field_index++;
			source++;
			field = dest;
			continue;
		}
		*dest++ = *source++; // No space or ", set dest to source and post-increment address
	}
	*dest = '\0';								  // set final address to null
	full_logs->length = dest - full_logs->buffer; // last memory address - first memory address = size
}

// Faster atoi conversion, less err checking overhead
inline int
fast_atoi(const char *str)
{
	int val = 0;
	while (*str >= '0' && *str <= '9') {
		val = val * 10 + (*str - '0');
		str++;
	}
	return val;
}

// Faster atoi conversion, for larger values like bytes_sent
inline int64_t
fast_atol(const char *str)
{
	int64_t val = 0;
	while (*str >= '0' && *str <= '9') {
		val = val * 10 + (*str - '0');
		str++;
	}
	return val;
}

// END - PARSE LOG LINE -> FULL LOG STRUCT ----------------------------


// EXTRACT FULL LOG -> SLIM LOG ---------------------------------------
void
extract_log_entry(p_log_t *full_log, s_log_t *slim_log, ip_track_t *ip_addrs)
{
	slim_log->timestamp = mktime(&full_log->time);
	slim_log->ip_hash = hash_key(&full_log->remote_ip);
	slim_log->key_hash = hash_key(&full_log->key);
	slim_log->podcast_hash = hash_podcast(&full_log->key);
	slim_log->bytes_sent_kb = (full_log->bytes_sent / 1024);
	slim_log->object_size_kb = (full_log->object_size / 1024);
	slim_log->download_time_ms = full_log->ms_ttime;
	slim_log->status_code = full_log->http_code;
	slim_log->platform_id = extract_platform(&full_log->user_agent);
	slim_log->device_type = extract_device(&full_log->user_agent);
	slim_log->country_id = extract_location(&full_log->remote_ip);
	slim_log->completion_percent = (((full_log->bytes_sent / full_log->object_size) / 1024) * 100);
	slim_log->flags = set_flags(&full_log, &slim_log, &ip_addrs);
}

uint32_t // /showname/#.mp3
hash_podcast(const char *key)
{
	size_t len = strlen(key);
	char temp[len + 1];
	char *source = key;
	char *dest = temp;
	uint32_t podcast_hash = DJB2HASH;

	if (*source == '/') {
		source++;
	}

	while (*source && *source != '/') {
		*dest++ = *source++;
	}
	*dest = '\0';

	return hash_key(temp);
}

uint32_t
hash_key(const char *key)
{
	uint32_t hash = DJB2HASH;
	for (const char *ptr = key; *ptr != '\0'; ptr++) {
		hash = (((hash << 5) + hash) + *ptr);
	}
	return hash;
}

uint8_t // "Spotify/8.8.4.669 Android/33 (SM-G781B)"
extract_platform(const char *user_agent)
{
}

uint8_t // "Spotify/8.8.4.669 Android/33 (SM-G781B)"
extract_device(const char *user_agent)
{
}

uint8_t
extract_location(const char *location)
{
}

uint8_t
set_flags(p_log_t *full_log, s_log_t *slim_log, ip_track_t *ip_addrs)
{
	uint8_t flags = 0;

	if (full_log->byte_start == 0) {
		flags = 0x02; // Set bit 00000010 to signify start of request

		if (is_unique_ip(ip_addrs, slim_log->ip_hash, slim_log->key_hash)) {
			flags = 0x03;
		}
	}
	if (full_log->byte_end >= (full_log->object_size - MEGABYTE)) {
		flags = 0x08; // Set bit 00001000 to signify end
	}
	else {
		flags = 0x04; // Set bit 00000100 to signify mid-portion
	}

	return flags;
}

int
is_unique_ip(ip_track_t *ip_addrs, uint32_t ip_hash, uint32_t key_hash)
{
	int flag = 0;



	return flag;
}

// END EXTRACT LOG FULL LOG -> SLIM LOG ------------------------------


// PROCESS SLIM LOG -> OUTPUT FILE
void
process_slim_logs(s_log_t *slim_log, int num_entrys, FILE *output)
{
}

// END PROCESS SLIM LOG -> OUTPUT
// END LOG PROCESSING DRIVER
