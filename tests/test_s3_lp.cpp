#include <gtest/gtest.h>
extern "C" {
#include "../include/s3_lp.h"
}

TEST(log_parser, MatchesStringCorrectly)
{
	char *str = "asdvS{pwdjaBlubrrywadjiadw";
	int result = check_pattern(str, "Blubrry");

	EXPECT_EQ(result, 1);
}
