# S3 Log Parser

A high-performance C tool for processing AWS S3 access logs with **95% compression ratio** and lightning-fast analytics capabilities. Built for podcast hosting companies and other S3-heavy applications where log processing performance directly impacts operational costs.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Key Features

- **Massive Compression**: Reduces S3 logs from ~500 bytes to 28 bytes per entry (95% reduction)
- **Lightning Fast**: Binary format enables rapid analytics queries without text parsing overhead
- **Memory Efficient**: Fixed-size binary records with predictable memory usage
- **Flexible Output**: Binary for storage, JSON for analysis and integration
- **Production Ready**: Batch processing, error handling, and scalable architecture

## Performance Benefits

| Metric | Raw S3 Logs | This Parser | Improvement |
|--------|-------------|-------------|-------------|
| Storage Size | 500+ bytes/entry | 28 bytes/entry | **95% reduction** |
| Query Speed | Text parsing required | Binary operations | **No parsing overhead** |
| Memory Usage | Unpredictable | Fixed 28B/record | **Predictable** |
| Processing | Text parsing overhead | Binary operations | **Minimal CPU** |

## Architecture

```
Raw S3 Logs → Parser → Compressed Binary → Extract Tool → JSON Analytics
     (~500B)         (~28B, 95% smaller)              (On-demand)
```

## Build & Install

```bash
# Clone the repository
git clone https://github.com/cochraneray/s3_log_parser.git
cd s3_log_parser

# Build all tools
make all

# Run demo
make demo
```

### Requirements
- GCC compiler
- Make
- Optional: Google Test for testing

## Usage

### 1. Parse S3 Logs to Binary Format

```bash
# Parse logs to compressed binary format
./s3lp -f access.log -o parsed.bin -v

# Options:
# -f <file>     Input S3 log file
# -o <file>     Output binary file  
# -v            Verbose output
# -t [bc]       Output type: (b)inary or (c)sv
```

### 2. Extract Binary to JSON for Analysis

```bash
# Extract all logs to JSON
./s3_extract -f parsed.bin -o output.json

# Group by podcast for show analytics  
./s3_extract -f parsed.bin -g p -o by_podcast.json

# Group by IP for user behavior analysis
./s3_extract -f parsed.bin -g i -o by_user.json

# Group by day for time-series analysis
./s3_extract -f parsed.bin -g t -o by_day.json

# Options:
# -f <file>     Input binary file
# -o <file>     Output JSON file
# -g [pitn]     Group by: (p)odcast, (i)p, (t)ime, (n)one
# -v            Verbose output
```

## File Structure

```
.
├── src/
│   ├── s3driver.c      # Main parser driver
│   ├── s3parser.c      # Core parsing logic
│   ├── s3extract.c     # JSON extraction tool
│   └── fake_logs.c     # Demo log generator
├── include/
│   ├── s3lp.h          # Main header
│   └── s3extract.h     # Extract tool header
├── tests/
│   └── test_parser.cpp # Unit tests
└── bin/                # Compiled binaries
```

## Demo & Testing

```bash
# Generate sample data and run complete pipeline
make demo

# Test with real data
./fake_logs                           # Creates fake_s3.log
./s3lp -f fake_s3.log -o test.bin -v  # Parse to binary
./s3_extract -f test.bin -g p -o podcasts.json  # Extract grouped by podcast

# Run unit tests
make testers
```

## Data Structure

### Input: Standard S3 Access Log Format
```
bucket_owner bucket [timestamp] remote_ip requester request_id operation key request_uri http_code error_code bytes_sent object_size total_time turn_around_time referer user_agent version_id host_id signature_version cipher_suite authentication_type host_header tls_version access_point_arn acl_required range_header
```

### Output: Compressed 28-Byte Binary Record
```c
typedef struct s_log_s {
    uint32_t timestamp;         // Unix timestamp
    uint32_t ip_hash;           // Hashed IP address
    uint32_t podcast_hash;      // Hashed podcast/show name
    uint32_t key_hash;          // Hashed full key
    uint16_t bytes_sent_kb;     // Bytes sent (KB)
    uint16_t object_size_kb;    // Object size (KB)
    uint16_t download_time_ms;  // Download time (ms)
    uint8_t http_code;          // HTTP status code
    uint8_t system_id;          // Platform (Spotify, Apple, etc.)
    uint8_t platform_id;        // Device/OS info
    uint8_t completion_percent; // Download completion %
    uint8_t flags;              // Partial download flags
} s_log_t;
```

### JSON Output Example
```json
{
  "grouped_by": "podcast",
  "groups": {
    "a1b2c3d4": {
      "count": 1524,
      "logs": [
        {
          "timestamp": 1714521600,
          "time": "2024-05-01 00:00:00",
          "ip_hash": "7f3e8901",
          "podcast_hash": "a1b2c3d4",
          "bytes_sent_kb": 15360,
          "object_size_kb": 15360,
          "completion_percent": 100,
          "http_code": 200
        }
      ]
    }
  }
}
```

## Use Cases

### Podcast Hosting Analytics
- **Download Analytics**: Track episode popularity and completion rates
- **User Behavior**: Analyze listening patterns and user retention
- **Geographic Insights**: Map listener distribution (with IP geolocation)
- **Platform Analytics**: Compare performance across Spotify, Apple, etc.

### S3 Cost Optimization
- **Storage Efficiency**: 95% reduction in log storage costs
- **Compute Savings**: Binary format eliminates text parsing overhead for analytics queries
- **Real-time Dashboards**: Enable live analytics without performance penalties

### Business Intelligence
- **Revenue Optimization**: Identify high-value content and user segments
- **Content Strategy**: Data-driven decisions on podcast content and scheduling
- **Technical Insights**: Download performance and infrastructure optimization

## Advanced Features

### Partial Download Handling
- Automatically tracks HTTP 206 (partial content) requests
- Flags start, middle, and end segments of chunked downloads
- Prevents double-counting in analytics

### Hash-Based Deduplication
- IP + content hash tracking for unique user identification
- Efficient memory usage with hash table collision handling
- Configurable hash table sizes for different scales

### Scalable Architecture
- Batch processing for memory efficiency
- Configurable batch sizes based on available memory
- Error handling and graceful degradation

## Performance Benchmarks

*Tested on 1M log entries (1.2GB raw S3 logs)*

### Parsing Performance
```
Raw log processing: 7.90 seconds
Memory usage: 14.7MB peak
Output size: 26.7MB (95% compression from 1.2GB)
```

### Extraction Performance
```bash
# Basic JSON extraction (1M entries)
./s3_extract -f logs.bin -o output.json
Time: 2.64 seconds
Memory: 95MB peak

# Grouped by podcast (20 shows)  
./s3_extract -f logs.bin -g p -o by_podcast.json
Time: 2.63 seconds
Memory: 123MB peak

# Grouped by IP address
./s3_extract -f logs.bin -g i -o by_ip.json  
Time: 2.74 seconds
Memory: 124MB peak
```

### Performance Summary
| Operation | Time | Memory | Notes |
|-----------|------|---------|-------|
| Parse 1M logs | 7.9s | 14.7MB | 1.2GB → 26.7MB |
| Extract to JSON | 2.6s | 95MB | All entries |
| Group by podcast | 2.6s | 123MB | 20 groups |
| Group by IP | 2.7s | 124MB | ~255k unique IPs |

**Total pipeline**: 1.2GB raw logs → parsed & analyzed in **~10 seconds**

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contact

Built by Ray Cochrane - rayhcochrane@gmail.com

Project Link: [https://github.com/cochraneray/s3_log_parser](https://github.com/cochraneray/s3_log_parser)

---

**Ready to make your S3 log analytics 10x faster? Clone and try the demo!**
