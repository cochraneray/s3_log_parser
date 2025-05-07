#include <gtest/gtest.h>
extern "C" {
#include "../include/s3_lp.h"
}

// SET FLAGS TESTS---------------------------------------------------------------
// Tests if start of file requests get set to correct bit -
// 0x01 FOR UNIQUE IP -> 0x02 For Start of Request
// Results in 0x03 if both flags are set
TEST(extract_utils, CorrecltySetsFlags206Strt)
{
	s_log_t slim_log;
	p_log_t full_log;

	s_context_t context;
	context.ip_track.capacity = 1;
	context.ip_track.count = 0;
	context.ip_track.ip_hashes = (uint64_t *)calloc(context.ip_track.capacity, sizeof(uint64_t));
	uint32_t ip = 0;
	uint32_t key = 1;

	// Test Multi-File download Flag setting
	slim_log.ip_hash = ip;
	slim_log.key_hash = key;
	slim_log.http_code = 206;

	full_log.byte_start = 0;
	full_log.byte_end = 1024;
	full_log.object_size = 4096;

	uint8_t result = set_flags(&full_log, &slim_log, &context);
	EXPECT_EQ(result, 0x03);
	free(context.ip_track.ip_hashes);
}

// Tests if middle of file requests get set to correct bit - 0x04
TEST(extract_utils, CorrecltySetsFlags206Mid)
{
	s_log_t slim_log;
	p_log_t full_log;

	s_context_t context;
	context.ip_track.capacity = 1;
	context.ip_track.count = 0;
	context.ip_track.ip_hashes = (uint64_t *)calloc(context.ip_track.capacity, sizeof(uint64_t));
	uint32_t ip = 0;
	uint32_t key = 1;

	// Test Multi-File download Flag setting
	slim_log.ip_hash = ip;
	slim_log.key_hash = key;
	slim_log.http_code = 206;
	full_log.byte_start = 2048;
	full_log.byte_end = 2048;
	full_log.object_size = 4096;

	uint8_t result = set_flags(&full_log, &slim_log, &context);
	EXPECT_EQ(result, 0x04);
	free(context.ip_track.ip_hashes);
}

TEST(extract_utils, CorrecltySetsFlags206End)
{
	s_log_t slim_log;
	p_log_t full_log;

	s_context_t context;
	context.ip_track.capacity = 2;
	context.ip_track.count = 0;
	context.ip_track.ip_hashes = (uint64_t *)calloc(context.ip_track.capacity, sizeof(uint64_t));
	uint32_t ip = 0;
	uint32_t key = 1;

	// Test Multi-File download Flag setting
	slim_log.ip_hash = ip;
	slim_log.key_hash = key;
	slim_log.http_code = 206;


	full_log.byte_start = 3001;
	full_log.byte_end = 4096;
	full_log.object_size = 4096;

	uint8_t result = set_flags(&full_log, &slim_log, &context);
	EXPECT_EQ(result, 0x08);
	free(context.ip_track.ip_hashes);
}

// SET FLAGS TESTS---------------------------------------------------------------
// IS UNIQUE IP TESTS------------------------------------------------------------
TEST(extract_utils, InsertsHashedIPCorrectly)
{
	s_context_t context;
	context.ip_track.capacity = 1;
	context.ip_track.count = 0;
	context.ip_track.ip_hashes = (uint64_t *)calloc(context.ip_track.capacity, sizeof(uint64_t));
	uint32_t ip = 0;
	uint32_t key = 1;

	int result = is_unique_ip(ip, key, &context);

	EXPECT_EQ(result, 1);
	EXPECT_EQ(context.ip_track.count, 1);

	// Insert based on hash % capacity
	EXPECT_EQ(context.ip_track.ip_hashes[0], 1);

	free(context.ip_track.ip_hashes);
}

// Ensures linear insert working correctly
TEST(extract_utils, InsertsHashLinearlyCorrectly)
{
	s_context_t context;
	context.ip_track.capacity = 5;
	context.ip_track.count = 0;
	context.ip_track.ip_hashes = (uint64_t *)calloc(context.ip_track.capacity, sizeof(uint64_t));
	uint32_t ip1 = 0;
	uint32_t key1 = 1; // 1 % 5 = 1 -> Will attempt to fill bucket[1]

	uint32_t ip2 = 0;
	uint32_t key2 = 6; // 6 % 5 = 1 -> Will attempt to fill bucket[1]
					   // Should Correctly fill next bucket instead bucket[2]

	int result = is_unique_ip(ip1, key1, &context);
	int result2 = is_unique_ip(ip2, key2, &context);

	EXPECT_EQ(result, 1);
	EXPECT_EQ(result2, 1);

	EXPECT_EQ(context.ip_track.ip_hashes[1], 1); // Check bucket 1 for val
	EXPECT_EQ(context.ip_track.ip_hashes[2], 6); // Check bucket 2 for val
	free(context.ip_track.ip_hashes);
}

// IS UNIQUE IP TESTS------------------------------------------------------------
// CHECK PATTERN TESTS------------------------------------------------------------
TEST(extract_utils, MatchesPatternCorrectly)
{
	char string[32] = "asdvS{pwdjaBlubrrywadjiadw";
	char *str = string;
	int result = check_pattern(str, "Blubrry");

	EXPECT_EQ(result, 1); // Expect Success
}

TEST(extract_utils, FailsPatternGracefully)
{
	char string[32] = "Rwdjwa9dwajdwjRwaVoice";
	char *str = string;
	int result = check_pattern(str, "Raw Voice");

	EXPECT_EQ(result, 0); // Expect Failure
}

TEST(extract_utils, FailsPartialPatternGracefully)
{
	char string[32] = "BadStringEndHallow";
	char *str = string;
	int result = check_pattern(str, "Halloween");

	EXPECT_EQ(result, 0); // Expect Failure
}

// CHECK PATTERN TESTS------------------------------------------------------------
