#include "../include/s3_lp.h"

// LOG PROCESSING DRIVER -----------------------------------------------------------------------
// process_log
// Iniitalizes array of structs (up to batch size) to intake logs, process and
// store When we reach BATCH SIZE we reset by processing the batch
int
process_log(FILE *log, FILE *output, s_context_t *context)
{
	char log_entry[LOG_DEFAULT];
	// Initialize batch_parsed_logs to hold BATCH SIZE addresses to p_log_t
	p_log_t *batch_parsed_logs = (p_log_t *)calloc(BATCH_SIZE, sizeof(p_log_t));

	// Exiting here was causing seg fault due to ip_tracker
	// Check not null
	if (batch_parsed_logs == NULL) {
		perror("Process Log: Malloc");
		return 1;
	}
	// Initialize slim logs to hold the same amount of addresses
	s_log_t *batch_slim_logs = (s_log_t *)calloc(BATCH_SIZE, sizeof(s_log_t));

	// exiting here was causing seg fault due to ip_tracker
	// Check not null
	if (batch_slim_logs == NULL) {
		perror("Process Log: Malloc");
		return 1;
	}

	// Initiailze a total counter and batch counter
	int count = 0;
	int total_processed = 0;

	// Process the logfile line by line
	while (fgets(log_entry, sizeof(log_entry), log)) {
		parse_log_entry(log_entry, &batch_parsed_logs[count], context);
		extract_log_entry(&batch_parsed_logs[count], &batch_slim_logs[count], context);
		++count;

		// Process batch when we've reached a critical point
		// Pre-Processing sucessful -> Empty into output file
		if (count >= BATCH_SIZE) {
			process_slim_logs(batch_slim_logs, count, output, context);
			total_processed += count;
			count = 0;
		}
	}


	// Pre-Processing Sucessful -> Empty into output file
	if (count > 0) {
		process_slim_logs(batch_slim_logs, count, output, context);
	}
	// Check verbose flag
	if (context->verbose) {
		fprintf(stderr, "%d Lines Processed", total_processed);
	}
	free(batch_parsed_logs);
	free(batch_slim_logs);
	return 0;
}

// PARSE LOG LINE -> FULL LOG STRUCT
// ----------------------------------------------------------------------
// Use fgets for line by line reads
void
parse_log_entry(char *in_log, p_log_t *full_logs, s_context_t *context)
{
	char *source = in_log;
	char *dest = full_logs->buffer;
	char *field = full_logs->buffer;
	int field_index = 0;
	int in_quote = 0;
	int in_bracket = 0;

	// While source address isnt null
	// fgets pulled until newline in process log
	while (*source && field_index <= 26) {
		// Quote Checker
		if (*source == '"') {
			in_quote = !in_quote;
			*dest++ = *source++;
			continue;
		}
		// Check for bracket,
		// delimit after like quotes
		if (*source == '[') {
			in_bracket++;
			*dest++ = *source++;
			continue;
		}
		if (*source == ']') {
			in_bracket--;
			*dest++ = *source++;
			continue;
		}


		// Space Delimiter indicates new field
		if (*source == ' ' && !in_quote && !in_bracket) {
			*dest++ = '\0'; // Set Dest address that holds space to \0 and post-increment


			// Assigns field based on field index / spot in the log
			// can only due this due to consistent rows
			// would likely have to change to be applicable for business
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
				char *strip_time;
				char time_buffer[32]; // allocate time buffer
				struct tm temp_time = {0};
				size_t i = 0;

				const char *start = field + 1;
				while (*start && *start != ']' && i < sizeof(time_buffer) - 1) {
					time_buffer[i++] = *start++;
				}
				time_buffer[i] = '\0';
				// strip time and fille temp_time

				strip_time = strptime(time_buffer, "%d/%b/%Y:%H:%M:%S %z", &temp_time);
				// Check for success
				if (strip_time != NULL) {
					full_logs->time = temp_time;
				}
				else {
					// set time struct to 0, error
					memset(&full_logs->time, 0, sizeof(full_logs->time));
					fprintf(stderr, "Failed to set time for: %d\n", field_index);
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
			// Particular to business s3 setup
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
				break;
			default:
				fprintf(stderr, "Error, outside the acceptable bounds of this intake function");
			}
			field_index++; // When field is set we need to increment
			source++;	   // Increment source separately

			// Set field to dest current address (start of next log field)
			field = dest;
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

		if (field_index > 26) {
			break;
		}
		// No space or ", set dest to source and post-increment address
		*dest++ = *source++;
	}
	*dest = '\0';								  // set final address to null
	full_logs->length = dest - full_logs->buffer; // last memory address - first memory address = size
	if (context->verbose) {
		fprintf(stderr, "Log->Struct:%s %s FieldInd:%d http:%d\n", full_logs->bucket_name, full_logs->key, field_index, full_logs->http_code);
	}
}

// END - PARSE LOG LINE -> FULL LOG STRUCT ----------------------------

// EXTRACT FULL LOG -> SLIM LOG ---------------------------------------
// Build slim log from info extracted from full log
void
extract_log_entry(p_log_t *full_log, s_log_t *slim_log, s_context_t *context)
{
	slim_log->timestamp = mktime(&full_log->time);
	slim_log->ip_hash = hash_key(full_log->remote_ip);
	slim_log->key_hash = hash_key(full_log->key);
	slim_log->podcast_hash = hash_podcast(full_log->key);
	slim_log->bytes_sent_kb = (full_log->bytes_sent / 1024);
	slim_log->object_size_kb = (full_log->object_size / 1024);
	slim_log->download_time_ms = full_log->ms_ttime;
	slim_log->http_code = full_log->http_code;
	slim_log->system_id = extract_system(full_log->user_agent);
	slim_log->platform_id = extract_platform(full_log->user_agent);
	slim_log->completion_percent = (full_log->object_size == 0) ? 0 : (100 * full_log->bytes_sent) / full_log->object_size;

	// Only need to call set flags if its a multi-file DL
	slim_log->flags = set_flags(full_log, slim_log, context);

	if (context->verbose) {
		time_t t;
		struct tm *local_time;
		t = time(NULL);
		local_time = localtime(&t);
		fprintf(stderr, "Full->Slim:[%02d:%02d:%02d] %x\n", local_time->tm_hour, local_time->tm_min, local_time->tm_sec, slim_log->key_hash);
	}
}

// DJB2 hashing method to organize podcasts
uint32_t // /showname/#.mp3
hash_podcast(const char *key)
{
	if (key == NULL || *key == '\0') {
		return DJB2HASH;
	}
	size_t key_len = strlen(key);
	char temp[key_len + 1];
	const char *source = key;
	char *dest = temp;

	if (*source == '/') {
		source++;
	}

	while (*source && (*source != '/')) {
		*dest++ = *source++;
	}
	*dest = '\0';

	return hash_key(temp);
}

// DJB2 Hashing methodology
uint32_t
hash_key(const char *key)
{
	if (key == NULL)
		return DJB2HASH;

	uint32_t hash = DJB2HASH;
	for (const char *ptr = key; *ptr != '\0'; ptr++) {
		hash = (((hash << 5) + hash) + *ptr);
	}
	return hash;
}

// User agent format:
// https://www.geeksforgeeks.org/http-headers-user-agent/
/*
 *User-Agent:
 *'system/<version> (<system-information>)'
 *' <platform>' (<platform-details>)'
 *' <extensions>'
 */
uint8_t // "Spotify/8.8.4.669 Android/33 (SM-G781B)"
extract_system(const char *user_agent)
{
	if (check_pattern(user_agent, "RawVoice Generator/"))
		return BLUBRRY;
	else if (check_pattern(user_agent, "Spotify/"))
		return SPOTIFY;
	else if (check_pattern(user_agent, "AppleCoreMedia/"))
		return APPLE_PODCASTS;
	else if (check_pattern(user_agent, "Googlebot/"))
		return GOOGLE_PODCASTS;
	else if (check_pattern(user_agent, "Youtube/"))
		return YOUTUBE;
	else
		return UNKNOWN;
}

// Uses check_pattern to find particular standout terms.
// Generic types based on popular brands and choices
// codes are all record in enum in header file
uint8_t // "Spotify/8.8.4.669 Android/33 (SM-G781B)"
extract_platform(const char *user_agent)
{
	uint16_t platform = DEV_UNKNOWN | OS_UNKNOWN;

	// Get device OS
	// Android
	if (check_pattern(user_agent, "Android")) {
		platform = (platform & 0xFF) | OS_ANDROID;
	}

	// iOS
	else if (check_pattern(user_agent, "iPhone") || check_pattern(user_agent, "iPad") || check_pattern(user_agent, "iOS")) {
		platform = (platform & 0xFF) | OS_IOS;
	}

	// Windows
	else if (check_pattern(user_agent, "Windows")) {
		platform = (platform & 0xFF) | OS_IOS;
	}

	// Macintosh
	else if (check_pattern(user_agent, "Macintosh") || check_pattern(user_agent, "Mac")) {
		platform = (platform & 0xFF) | OS_MACOS;
	}

	// Linux
	else if (check_pattern(user_agent, "tvOS")) {
		platform = (platform & 0xFF) | OS_TV;
	}

	// Smart Watch
	else if (check_pattern(user_agent, "watchOS")) {
		platform = (platform & 0xFF) | OS_WATCH;
	}

	// Get device type
	// Check Watch
	if (platform & OS_WATCH) {
		platform = (platform & 0xFF00) | DEV_WATCH;
	}

	// Check TV
	else if (platform & OS_TV) {
		platform = (platform & 0xFF00) | DEV_TV;
	}

	// Check Mobile
	else if (check_pattern(user_agent, "Mobile") || (platform & OS_IOS && check_pattern(user_agent, "iPhone"))) {
		platform = (platform & 0xFF00) | DEV_MOBILE;
	}

	// Check Tablet
	else if (check_pattern(user_agent, "Tablet") || check_pattern(user_agent, "iPad")) {
		platform = (platform & 0xFF00) | DEV_TABLET;
	}

	// Check Smart Speakers
	else if (check_pattern(user_agent, "Echo") || check_pattern(user_agent, "HomePod") || check_pattern(user_agent, "GoogleHome")) {
		platform = (platform & 0xFF00) | DEV_SMART_SPEAKER;
	}

	// Check Desktop
	else if (platform & (OS_WINDOWS | OS_LINUX | OS_MACOS) && (check_pattern(user_agent, "Mobile") == 1)) {
		platform = (platform & 0xFF00) | DEV_DESKTOP;
	}

	return platform;
}

// Using bitflags for true false like file permissions, 8 bits to represent true false for different conditions
// Checks htpp_code to change how it interacts with flags, only have multi-request downloads currently
//
// Should probably swap this into an enum for clarity
uint8_t
set_flags(p_log_t *full_log, s_log_t *slim_log, s_context_t *context)
{
	uint8_t flags = 0;
	http_flag_t http_flag = DEFAULT;
	size_t end_check = MEGABYTE;
	// 206 indicates multi-request for downloads, flag system allows us to avoid
	// counting the same ip several times
	if (slim_log->http_code == 206) {
		if (full_log->byte_start == 0) {
			flags |= STRT_206DL; // Set bit 00000010 to signify start of request

			// Check if ip is unique against ip_tracker
			// Might want another wrapper around is_unique_ip that updates a analytics struct
			if (1 == is_unique_ip(slim_log->ip_hash, slim_log->key_hash, context)) {
				flags |= UNIQUE_IP; // Set bit 00000011
			}
		}
		// Reduce End of file checker if total size is less than 1mb
		if (full_log->object_size < end_check) {
			end_check = fsize_KB;
		}

		// Check if within a megabyte of the end of the file to guestimate end
		if (full_log->byte_end >= (full_log->object_size - end_check)) {
			flags = END_206DL; // Set bit 00001000 to signify end
		}
		else if (flags == 0) {
			flags = MID_206DL; // Set bit 00000100 to signify mid-portion of file
		}
	}


	if (context->verbose == 1) {
		fprintf(stderr, "%u: Flag Set\n", slim_log->key_hash);
	}
	return flags;
}

int
is_unique_ip(uint32_t ip_hash, uint32_t key_hash, s_context_t *context)
{
	// Combine ip hash and key hash to generate unique id
	uint64_t complete_hash = (((uint64_t)ip_hash << 32) | key_hash);

	// Ensure Hash isnt 0
	if (complete_hash == 0) {
		complete_hash = 1;
	}

	// Normalize that Hash to be within index range
	uint32_t index = complete_hash % context->ip_track.capacity; // Capacity is Hashing value
	uint32_t original_index = index;

	do {
		// Index empty
		if ((context->ip_track.ip_hashes[index]) == 0) {
			context->ip_track.ip_hashes[index] = complete_hash;
			context->ip_track.count += 1;
			return 1;
		}
		// Already recorded, skip
		if (context->ip_track.ip_hashes[index] == complete_hash) {
			return 0;
		}
		// Try next index, if index was miraculously IP_HASH we return back to 0
		// then 1
		index = (index + 1) % IP_HASH;
	} while (index != original_index);
	return 0;
}

// Pattern Matching using char checker for flag matching
int
check_pattern(const char *check_str, const char *pattern)
{
	// Search for first char, check rest for match
	for (size_t i = 0; check_str[i] != '\0'; i++) {
		if (check_str[i] == pattern[0]) {
			int match = 1; // Matches
			for (size_t j = 0; pattern[j] != '\0'; j++) {
				if (check_str[i + j] == '\0' || check_str[i + j] != pattern[j]) {
					match = 0; // No Match
					break;
				}
			}
			if (match == 1) // Matches
				return 1;
		}
	}
	return 0; // No Match
}

// END EXTRACT LOG FULL LOG -> SLIM LOG ------------------------------

// PROCESS SLIM LOG -> OUTPUT FILE
void
process_slim_logs(s_log_t *slim_log, int num_entries, FILE *output, s_context_t *context)
{
	if (context->output_filetype_flag == CSV_FILE) {
		output_CSV(slim_log, num_entries, output, context);
	}
	else {
		for (int i = 0; i < num_entries; i++) {
			fwrite(&slim_log[i], sizeof(s_log_t), 1, output);
		}
		if (context->verbose) {
			fprintf(stderr, "Batch Extracted\n");
		}
	}
}

void
output_CSV(s_log_t *slim_log, int num_entries, FILE *output, s_context_t *context)
{
	fprintf(output, "timestamp,ip_hash,podcast_hash,key_hash"
					",bytes_sent_kb,object_size_kb,download_time_ms,"
					"status_code,system_id,platform-id,completion_percent,flags\n");

	for (int i = 0; i < num_entries; i++) {
		fprintf(output, "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", slim_log->timestamp, slim_log->ip_hash, slim_log->podcast_hash, slim_log->key_hash, slim_log->bytes_sent_kb,
				slim_log->object_size_kb, slim_log->download_time_ms, slim_log->http_code, slim_log->system_id, slim_log->platform_id, slim_log->completion_percent,
				slim_log->flags);
	}
	if (context->verbose) {
		fprintf(stderr, "Batch Extracted\n");
	}
}

// END PROCESS SLIM LOG -> OUTPUT
// END LOG PROCESSING DRIVER
