LDFLAGS=-pthread
CFLAGS=-std=gnu99 -pedantic -Wall -Wextra

ifndef RELEASE
CFLAGS+=-g
else
CFLAGS+=-O3 -Os
endif

all: libj conbot jbot

libj: libj.a
libj.a: lib/bmap.o lib/cbuffer.o
	$(AR) rcs libj.a $^

conbot: conbot.o libj.a ircsock.o
	$(CC) -o $@ $^ $(LDFLAGS)
jbot: jbot.o libj.a util.o functions.o greetings.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f lib/*.o libj.a
	rm -f *.o conbot jbot
