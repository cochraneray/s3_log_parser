CC = gcc
CXX = g++
CCFLAGS = -g3 -Wall -Wextra -O3 -D_XOPEN_SOURCE
CXXFLAGS = -Wall -Iinclude
LDFLAGS = -lgtest -lgtest_main -lpthread

SRC_DIR = src
INCLUDE_DIR = include
TEST_DIR = tests

all: s3lp fake_logs test_s3lp

# Build main C binaries
s3lp: s3_lp.o
	$(CC) $(CCFLAGS) -o s3lp s3_lp.o -lc

s3_lp.o: $(SRC_DIR)/s3_lp.c $(INCLUDE_DIR)/s3_lp.h
	$(CC) $(CCFLAGS) -I$(INCLUDE_DIR) -c $(SRC_DIR)/s3_lp.c

fake_logs: fake_logs.o
	$(CC) $(CCFLAGS) -o fake_logs fake_logs.o

fake_logs.o: $(SRC_DIR)/fake_logs.c
	$(CC) $(CCFLAGS) -c $(SRC_DIR)/fake_logs.c

# Build GoogleTest test binary
test_s3lp: $(TEST_DIR)/test_s3_lp.cpp $(SRC_DIR)/s3_lp.c $(INCLUDE_DIR)/s3_lp.h
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/s3_lp.c $(TEST_DIR)/test_s3_lp.cpp -o test_s3lp $(LDFLAGS)


.PHONY: clean

clean:
	rm -f *.o *.bin *.log s3lp fake_logs test_s3lp
