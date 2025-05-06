CC = gcc
CCFLAGS = -g3 -Wall -Wextra -O3 -D_XOPEN_SOURCE

all: s3lp fake_logs

s3lp: s3_lp.o
	$(CC) $(CCFLAGS) -o s3lp s3_lp.o -lc

s3_lp.o: s3_lp.c s3_lp.h
	$(CC) $(CCFLAGS) -c s3_lp.c

fake_logs: fake_logs.o
	$(CC) $(CCFLAGS) -o fake_logs fake_logs.o

fake_logs.o: fake_logs.c
	$(CC) $(CCFLAGS) -c fake_logs.c


.PHONY: clean

clean:
	rm -f *.o *.bin *.log s3lp fake_logs
