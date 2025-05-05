// S3 Log Parser
//
//
//
#include "s3_lp.h"

// static struct for holding hashed ip + key combinations
static ip_track_t ip_track;

int main(int argc, char *argv[]) {
  char *filename = NULL;    // default input filename
  char *output_file = NULL; // default output filename
  FILE *ifp = stdin;        // default file path to stdin
  FILE *ofp = stdout;       // default file path to stdout

  // Initialize ip_hash struct
  {
    ip_track.capacity = IP_HASH;
    ip_track.count = 0;
    ip_track.ip_hashes = calloc(IP_HASH, sizeof(uint64_t));
  }
  // Getopt scope - OPTIONS in header file
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
        fprintf(stderr,
                "USAGE: ./s3_lp -[h] -f <filepath> -o <output_filepath>\n"
                "\t-f filepath: override default filepath from stdin\n"
                "\t-o filepath: override default output filepath from stdout\n"
                "\t-h display options\n");
        exit(EXIT_SUCCESS);
      }
      default:
        fprintf(stderr, "USAGE: ./s3_lp -[options] -f <filepath>\n\tUse -h to "
                        "get help:\n\n\t./s3_lp -h");
        exit(EXIT_SUCCESS);
      } // End of Switch
    } // End of getopt while loop
  } // end of getopt scope

  // if filename was passed in, replace FILE*
  if (filename != NULL) {
    ifp = fopen(filename, "r");
    if (ifp == NULL) {
      perror("fopen intake");
      exit(EXIT_FAILURE);
    }
  }

  // if output filename was passed in, replace FILE*
  if (output_file != NULL) {
    ofp = fopen(output_file, "w");
    if (ofp == NULL) {
      perror("fopen output");
      exit(EXIT_FAILURE);
    }
  }

  // run log processing
  process_log(ifp, ofp);

  // Free FILE* and struct
  free(ip_track.ip_hashes);
  fclose(ifp);
  fclose(ofp);
  exit(EXIT_SUCCESS);
}

// LOG PROCESSING DRIVER
// -----------------------------------------------------------------------
// process_log
// Iniitalizes array of structs (up to batch size) to intake logs, process and
// store When we reach BATCH SIZE we reset by processing the batch
void process_log(FILE *log, FILE *output) {
  char log_entry[LOG_SIZE];
  // Initialize batch_parsed_logs to hold BATCH SIZE addresses to p_log_t
  p_log_t *batch_parsed_logs = (p_log_t *)calloc(BATCH_SIZE, sizeof(p_log_t));
  // Check not null
  if (batch_parsed_logs == NULL) {
    perror("Process Log: Malloc");
    exit(EXIT_FAILURE);
  }
  // Initialize slim logs to hold the same amount of addresses
  s_log_t *batch_slim_logs = calloc(BATCH_SIZE, sizeof(s_log_t));
  // Check not null
  if (batch_slim_logs == NULL) {
    perror("Process Log: Malloc");
    exit(EXIT_FAILURE);
  }

  // Initiailze a total counter and batch counter
  int count = 0;
  int total_processed = 0;

  // Process the logfile line by line
  while (fgets(log_entry, sizeof(log_entry), log)) {
    parse_log_entry(log_entry, &batch_parsed_logs[count]);
    extract_log_entry(&batch_parsed_logs[count], &batch_slim_logs[count]);
    ++count;

    // Process batch when we've reached a critical point
    if (count >= BATCH_SIZE) {
      process_slim_logs(batch_slim_logs, count, output);
      total_processed += count;
      count = 0;
    }
  }

  if (count > 0) {
    process_slim_logs(batch_slim_logs, count, output);
  }

  fprintf(stderr, "%d Lines Processed", total_processed);

  free(batch_parsed_logs);
  free(batch_slim_logs);
}

// PARSE LOG LINE -> FULL LOG
// STRUCT--------------------------------------------------------
void parse_log_entry(char *in_log, p_log_t *full_logs) {
  char *source = in_log;
  char *dest = full_logs->buffer;
  char *field = full_logs->buffer;
  int field_index = 0;
  int in_quote = 0;
  int in_bracket = 0;

  // While source address isnt null (since fgets pulled until newline in process
  // log)
  while (*source && field_index <= 26) {
    // Quote Checker
    if (*source == '"') {
      in_quote = !in_quote;
      *dest++ = *source++;
      continue;
    }

    if (*source == '[') {
      in_bracket = 1;
      *dest++ = *source++;
      continue;
    }
    if (*source == ']') {
      in_bracket = 0;
      *dest++ = *source++;
      continue;
    }
    // Space Delimiter indicates new field
    if (*source == ' ' && !in_quote && !in_bracket) {
      *dest++ =
          '\0'; // Set Dest address that holds space to \0 and post-increment

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
        } else {
          // set time struct to 0, error
          memset(&full_logs->time, 0, sizeof(time));
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
        } else {
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
        fprintf(stderr,
                "Error, outside the acceptable bounds of this intake function");
      }
      field_index++; // When field is set we need to increment
      source++;      // Increment source separately

      // Set field to dest current address (start of next log field)
      field = dest;
      continue;
    }

    // Range field for partial downloads
    // https://docs.aws.amazon.com/AmazonCloudFront/latest/DeveloperGuide/RangeGETs.html
    if (*source == ' ' && field_index == 26 && full_logs->http_code == 206 &&
        !in_quote) {
      full_logs->range_get = field;
      sscanf(full_logs->range_get, "\"bytes%lu-%lu\"", &full_logs->byte_start,
             &full_logs->byte_end);

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
  *dest = '\0'; // set final address to null
  full_logs->length =
      dest -
      full_logs->buffer; // last memory address - first memory address = size
}

// END - PARSE LOG LINE -> FULL LOG STRUCT ----------------------------

// EXTRACT FULL LOG -> SLIM LOG ---------------------------------------
// Build slim log from info extracted from full log
void extract_log_entry(p_log_t *full_log, s_log_t *slim_log) {
  slim_log->timestamp = mktime(&full_log->time);
  slim_log->ip_hash = hash_key(full_log->remote_ip);
  slim_log->key_hash = hash_key(full_log->key);
  slim_log->podcast_hash = hash_podcast(full_log->key);
  slim_log->bytes_sent_kb = (full_log->bytes_sent / 1024);
  slim_log->object_size_kb = (full_log->object_size / 1024);
  slim_log->download_time_ms = full_log->ms_ttime;
  slim_log->status_code = full_log->http_code;
  slim_log->system_id = extract_system(full_log->user_agent);
  slim_log->platform_id = extract_platform(full_log->user_agent);
  slim_log->completion_percent =
      (full_log->object_size == 0)
          ? 0
          : (100 * full_log->bytes_sent) / full_log->object_size;

  // Only need to call set flags if its a multi-file DL
  if (slim_log->status_code == 206) {
    slim_log->flags = set_flags(full_log, slim_log);
  } else {
    slim_log->flags = 0;
  }
}

// DJB2 hashing method to organize podcasts
uint32_t // /showname/#.mp3
hash_podcast(const char *key) {
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

uint32_t hash_key(const char *key) {
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
extract_system(const char *user_agent) {
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

uint8_t // "Spotify/8.8.4.669 Android/33 (SM-G781B)"
extract_platform(const char *user_agent) {
  uint16_t platform = DEV_UNKNOWN | OS_UNKNOWN;

  // Get device OS
  // Android
  if (check_pattern(user_agent, "Android")) {
    platform = (platform & 0xFF) | OS_ANDROID;
  }

  // iOS
  else if (check_pattern(user_agent, "iPhone") ||
           check_pattern(user_agent, "iPad") ||
           check_pattern(user_agent, "iOS")) {
    platform = (platform & 0xFF) | OS_IOS;
  }

  // Windows
  else if (check_pattern(user_agent, "Windows")) {
    platform = (platform & 0xFF) | OS_IOS;
  }

  // Macintosh
  else if (check_pattern(user_agent, "Macintosh") ||
           check_pattern(user_agent, "Mac")) {
    platform = (platform & 0xFF) | OS_MACOS;
  }

  // Linux
  else if (check_pattern(user_agent, "tvOS")) {
    platform = (platform & 0xFF) | OS_TVOS;
  }

  // Smart Watch
  else if (check_pattern(user_agent, "watchOS")) {
    platform = (platform & 0xFF) | OS_WATCHOS;
  }

  // Get device type
  // Check Watch
  if (platform & OS_WATCHOS) {
    platform = (platform & 0xFF00) | DEV_WATCH;
  }
  // Check TV
  else if (platform & OS_TVOS) {
    platform = (platform & 0xFF00) | DEV_TV;
  }
  // Check Mobile
  else if (check_pattern(user_agent, "Mobile") ||
           (platform & OS_IOS && check_pattern(user_agent, "iPhone"))) {
    platform = (platform & 0xFF00) | DEV_MOBILE;
  }

  // Check Tablet
  else if (check_pattern(user_agent, "Tablet") ||
           check_pattern(user_agent, "iPad")) {
    platform = (platform & 0xFF00) | DEV_TABLET;
  }

  // Check Smart Speakers
  else if (check_pattern(user_agent, "Echo") ||
           check_pattern(user_agent, "HomePod") ||
           check_pattern(user_agent, "GoogleHome")) {
    platform = (platform & 0xFF00) | DEV_SMART_SPEAKER;
  }

  // Check Desktop
  else if (platform & (OS_WINDOWS | OS_LINUX | OS_MACOS) &&
           (check_pattern(user_agent, "Mobile") == 1)) {
    platform = (platform & 0xFF00) | DEV_DESKTOP;
  }

  return platform;
}

// Using bitflags for efficiency, 8 bits to represent true false
uint8_t set_flags(p_log_t *full_log, s_log_t *slim_log) {
  uint8_t flags = 0;

  if (full_log->byte_start == 0) {
    flags |= 0x02; // Set bit 00000010 to signify start of request

    // Check if ip is unique against ip_tracker
    if (0 == is_unique_ip(slim_log->ip_hash, slim_log->key_hash)) {
      flags |= 0x03; // Set bit 00000011
    }
  }
  // Check if within a megabyte of the end of the file to guestimate end
  if (full_log->byte_end >= (full_log->object_size - MEGABYTE)) {
    flags |= 0x08; // Set bit 00001000 to signify end
  } else {
    flags |= 0x04; // Set bit 00000100 to signify mid-portion of file
  }
  return flags;
}

int is_unique_ip(uint32_t ip_hash, uint32_t key_hash) {
  int flag = 0;
  // Combine ip hash and key hash to generate unique id
  uint64_t complete_hash = (((uint64_t)ip_hash << 32) | key_hash);

  // Ensure Hash isnt 0
  if (complete_hash == 0) {
    complete_hash = 1;
  }
  uint32_t index = complete_hash % IP_HASH;
  uint32_t original_index = index;

  do {
    // Index empty
    if ((ip_track.ip_hashes[index]) == 0) {
      ip_track.ip_hashes[index] = complete_hash;
      ip_track.count += 1;
      return 0;
    }
    // Already recorded, skip
    if (ip_track.ip_hashes[index] == complete_hash) {
      return 1;
    }
    // Try next index, if index was miraculously IP_HASH we return back to 0
    // then 1
    index = (index + 1) % IP_HASH;
  } while (index != original_index);
  return flag;
}

// Pattern Matching using char checker for flag matching
int check_pattern(const char *check_str, const char *pattern) {
  // Search for first char, check rest for match
  for (size_t i = 0; check_str[i] != '\0'; i++) {
    if (check_str[i] == pattern[0]) {
      int match = 1;
      for (size_t j = 0; pattern[j] != '\0'; j++) {
        if (check_str[i + j] == '\0' || check_str[i + j] != pattern[j]) {
          match = 0;
          break;
        }
      }
      if (match == 1)
        return 1;
    }
  }
  return 0;
}

// END EXTRACT LOG FULL LOG -> SLIM LOG ------------------------------

// PROCESS SLIM LOG -> OUTPUT FILE
void process_slim_logs(s_log_t *slim_log, int num_entries, FILE *output) {
  for (int i = 0; i < num_entries; i++) {
    fwrite(&slim_log[i], sizeof(s_log_t), 1, output);
  }
}

// END PROCESS SLIM LOG -> OUTPUT
// END LOG PROCESSING DRIVER
