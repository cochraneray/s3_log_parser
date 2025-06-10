CC = gcc
CXX = g++
CCFLAGS = -g3 -Wall -Wextra -O3 -D_XOPEN_SOURCE
CXXFLAGS = -Wall -Iinclude -Wno-deprecated-declarations
LDFLAGS = -lgtest -lgtest_main -lpthread
SRC_DIR = src
INCLUDE_DIR = include
TEST_DIR = tests
BIN_DIR = bin

# Object files
MAIN_OBJS = $(BIN_DIR)/s3driver.o $(BIN_DIR)/s3parser.o
EXTRACT_OBJS = $(BIN_DIR)/s3extract.o $(BIN_DIR)/s3parser.o
FAKE_OBJS = $(BIN_DIR)/fake_logs.o
TEST_OBJS = $(BIN_DIR)/s3parser.o

all: s3lp s3_extract fake_logs test_s3lp

# Ensure bin directory exists
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# MAIN PARSER
s3lp: $(BIN_DIR) $(MAIN_OBJS)
	$(CC) $(CCFLAGS) -o s3lp $(MAIN_OBJS) -lc

$(BIN_DIR)/s3driver.o: $(SRC_DIR)/s3driver.c $(INCLUDE_DIR)/s3lp.h | $(BIN_DIR)
	$(CC) $(CCFLAGS) -I$(INCLUDE_DIR) -c $(SRC_DIR)/s3driver.c -o $@

$(BIN_DIR)/s3parser.o: $(SRC_DIR)/s3parser.c $(INCLUDE_DIR)/s3lp.h | $(BIN_DIR)
	$(CC) $(CCFLAGS) -I$(INCLUDE_DIR) -c $(SRC_DIR)/s3parser.c -o $@

# EXTRACT TOOL
s3_extract: $(BIN_DIR) $(EXTRACT_OBJS)
	$(CC) $(CCFLAGS) -o s3_extract $(EXTRACT_OBJS) -lc

$(BIN_DIR)/s3extract.o: $(SRC_DIR)/s3extract.c $(INCLUDE_DIR)/s3extract.h $(INCLUDE_DIR)/s3lp.h | $(BIN_DIR)
	$(CC) $(CCFLAGS) -I$(INCLUDE_DIR) -c $(SRC_DIR)/s3extract.c -o $@

# FAKE LOGS
fake_logs: $(BIN_DIR)/fake_logs.o
	$(CC) $(CCFLAGS) -o fake_logs $^

$(BIN_DIR)/fake_logs.o: $(SRC_DIR)/fake_logs.c | $(BIN_DIR)
	$(CC) $(CCFLAGS) -c $< -o $@

# TESTS
testers: test_s3lp
	./test_s3lp

test_s3lp: $(TEST_DIR)/test_parser.cpp $(BIN_DIR)/s3parser.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# TESTING PIPELINE
test_pipeline: s3lp s3_extract fake_logs
	@echo "Testing complete pipeline..."
	./fake_logs
	./s3lp -f fake_s3.log -o out/tests/test_output.bin -v
	./s3_extract -f out/tests/test_output.bin -o out/tests/test_output.json -v
	@echo "Pipeline test complete! Check test_output.json"

# DEMO - creates sample data and shows both binary and JSON output
demo: s3lp s3_extract fake_logs
	@echo "=== S3 Log Parser Demo ==="
	@echo -e "1. Generating sample logs.."
	./fake_logs
	@echo -e "\n\n2. Parsing to binary format..."
	/bin/time ./s3lp -f fake_s3.log -o out/bin/demo_output.bin 
	@echo -e "\n"
	/bin/time ./s3_extract -f out/bin/demo_output.bin -o out/json/demo_all.json
	@echo -e "\n"
	/bin/time ./s3_extract -f out/bin/demo_output.bin -g p -o out/json/demo_by_podcast.json
	@echo -e "\n"
	/bin/time ./s3_extract -f out/bin/demo_output.bin -g i -o out/json/demo_by_ip.json
	@echo -e "\n\n4. Demo files created:"
	@ls -lah out/bin/demo_*
	@ls -lah out/json/demo_*
	@echo "=== Demo Complete ==="

.PHONY: clean testers test_pipeline demo

clean:
	rm -f $(BIN_DIR)/*.o *.bin *.log *.json s3lp s3_extract fake_logs test_s3lp
	rm -f out/tests/test_* out/bin/demo_* out/json/demo_*
