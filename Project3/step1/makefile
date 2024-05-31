SRC = $(wildcard *.c) $(wildcard ../lib/*.c)
LIBS = ../lib/tcp_buffer.o ../lib/tcp_utils.o ../lib/thpool.o
CC = gcc
CFLAGS += -I../include -Wall -Wextra -fsanitize=address -lpthread -g

all: BDS BDC_command BDC_random

BDS: BDS.o $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
	
BDC_command: BDC_command.o $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

BDC_random: BDC_random.o $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

include ${SRC:.c=.d}
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -f *.o *.d BDS BDC_command BDC_random
	rm kk.txt
.PHONY: all clean
