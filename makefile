CC = gcc
CCFLAGS = -g3 -Wall

all: pclf

pclf: pclf.o
	$(CC) $(CCFLAGS) -o pclf pclf.o -lc

pclf.o: pclf.c pclf.h
	$(CC) $(CCFLAGS) -c pclf.c 

.PHONY: clean

clean:
	rm -f *.o pclf
