#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LOGS_TO_MAKE 1000000
#define NUM_PODCASTS 20

// Array of realistic podcast names
const char *podcast_names[NUM_PODCASTS] = {
	"tech-talk",	   "daily-news",	 "comedy-hour",	  "true-crime",		   "history-deep-dive",
	"startup-stories", "music-reviews",	 "book-club",	  "fitness-tips",	   "cooking-show",
	"travel-tales",	   "science-corner", "movie-reviews", "language-learning", "meditation-guide",
	"sports-weekly",   "art-spotlight",	 "gaming-news",	  "health-matters",	   "finance-focus"};

int
main()
{
	FILE *f = fopen("fake_s3.log", "w");
	if (!f) {
		perror("Failed to open file");
		return 1;
	}

	srand(time(NULL)); // Add some randomness

	for (int i = 0; i < LOGS_TO_MAKE; i++) {
		int id = i;
		int second = i % 60;
		int minute = (i / 60) % 60;
		int hour = (i / 3600) % 24;

		// Select a podcast (distribute roughly evenly but with some randomness)
		int podcast_id = (i / 100) % NUM_PODCASTS; // Change podcast every 100 entries
		if (rand() % 10 == 0) {					   // 10% chance to pick random podcast
			podcast_id = rand() % NUM_PODCASTS;
		}
		const char *podcast_name = podcast_names[podcast_id];

		// Episode number within that podcast
		int episode_num = (i / NUM_PODCASTS) + 1;

		// Determine if this should be a partial download (206)
		int is_partial = (i % 7 == 0); // ~14% partial downloads
		int http_code = is_partial ? 206 : 200;

		// Vary object sizes and bytes sent
		int object_size = 15000000 + (rand() % 10000000); // 15-25MB files
		int bytes_sent;

		if (is_partial) {
			bytes_sent = rand() % object_size; // Partial download
		}
		else {
			bytes_sent = object_size; // Complete download
		}

		fprintf(f,
				"79a59df900b949e5 "						  // bucket_owner
				"bucket%d "								  // bucket_name
				"[03/May/2025:%02d:%02d:%02d +0000] "	  // time
				"203.0.113.%d "							  // remote_ip
				"arn:aws:iam::123456789012:user/test-%d " // requester_id
				"EXAMPLEID%d "							  // request_id
				"REST.GET.OBJECT "						  // operation
				"%s/episode-%d.mp3 "					  // key (podcast_name/episode-N.mp3)
				"\"GET /%s/episode-%d.mp3 HTTP/1.1\" "	  // request_uri
				"%d "									  // http_code (200 or 206)
				"- "									  // err_code
				"%d "									  // bytes_sent (variable)
				"%d "									  // object_size (variable)
				"%d "									  // ms_ttime (variable)
				"42 "									  // ms_tatime
				"\"-\" "								  // referer
				"\"FakeAgent/1.0\" "					  // user_agent
				"v%d "									  // ver_id
				"HOSTID%d "								  // host_id
				"SigV2 "								  // auth_sig
				"ECDHE-RSA-AES128-GCM-SHA256 "			  // cipher_suite
				"AuthHeader "							  // auth_type
				"host%d.example.com "					  // host_header
				"TLSv1.2 "								  // TLS_ver
				"arn:aws:s3:::example-AP%d "			  // ARN_ap
				"false",								  // acl_required
				id % 1000, hour, minute, second,		  // bucket, time fields
				id % 255, id % 100, id,					  // IP, requester, request_id
				podcast_name, episode_num,				  // key fields (podcast/episode)
				podcast_name, episode_num,				  // request_uri fields
				http_code,								  // http_code
				bytes_sent, object_size,				  // bytes_sent, object_size
				20 + (rand() % 100),					  // ms_ttime
				id % 500, id % 50,						  // ver_id, host_id
				id % 10, id % 100);						  // host_header, ARN_ap

		// Add range field ONLY for 206 responses
		if (is_partial) {
			int start_byte = rand() % (object_size / 2);
			int end_byte = start_byte + bytes_sent - 1;
			fprintf(f, " \"bytes=%d-%d\"", start_byte, end_byte);
		}

		fprintf(f, "\n");
	}

	fclose(f);
	printf("Generated %d log entries across %d different podcasts with ~%d partial downloads (206 responses)\n",
		   LOGS_TO_MAKE, NUM_PODCASTS, LOGS_TO_MAKE / 7);

	// Print podcast distribution info
	printf("Podcasts created:\n");
	for (int i = 0; i < NUM_PODCASTS; i++) {
		printf("  %s\n", podcast_names[i]);
	}

	return 0;
}
