CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -O2
LDFLAGS = -lpthread

OBJS = main.o buffer.o workers.o log.o

all: sum_system

sum_system: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) sum_system

.PHONY: all clean
