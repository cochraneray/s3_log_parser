#include <stdio.h>

#define LOGS_TO_MAKE 1000000

int
main()
{
	FILE *f = fopen("fake_s3.log", "w");
	if (!f) {
		perror("Failed to open file");
		return 1;
	}

	for (int i = 0; i < LOGS_TO_MAKE; i++) {
		int id = i;
		int second = i % 60;
		int minute = (i / 60) % 60;
		int hour = (i / 3600) % 24;

		fprintf(f,
				"79a59df900b949e5 "							  // bucket_owner
				"bucket%d "									  // bucket_name
				"[03/May/2025:%02d:%02d:%02d +0000] "		  // time
				"203.0.113.%d "								  // remote_ip
				"arn:aws:iam::123456789012:user/test-%d "	  // requester_id
				"EXAMPLEID%d "								  // request_id
				"REST.GET.OBJECT "							  // operation
				"episodes/show%d/ep%d.mp3 "					  // key
				"\"GET /episodes/show%d/ep%d.mp3 HTTP/1.1\" " // request_uri
				"200 "										  // http_code
				"- "										  // err_code
				"18432056 "									  // bytes_sent
				"18432056 "									  // object_size
				"43 "										  // ms_ttime
				"42 "										  // ms_tatime
				"\"-\" "									  // referer
				"\"FakeAgent/1.0\" "						  // user_agent
				"v%d "										  // ver_id
				"HOSTID%d "									  // host_id
				"SigV2 "									  // auth_sig
				"ECDHE-RSA-AES128-GCM-SHA256 "				  // cipher_suite
				"AuthHeader "								  // auth_type
				"host%d.example.com "						  // host_header
				"TLSv1.2 "									  // TLS_ver
				"arn:aws:s3:::example-AP%d "				  // ARN_ap
				"false",									  // acl_required
				id % 1000, hour, minute, second, id % 255, id % 100, id, id % 50, id % 1000, id % 50, id % 1000, id % 50, id % 500, id % 50, id % 10);

		// Add range GET field (field 26) only for partial responses (e.g. 206)
		if (i % 5 == 0) {
			fprintf(f, " \"bytes=0-1023\"");
		}

		fprintf(f, "\n");
	}

	fclose(f);
	return 0;
}
