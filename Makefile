LDFLAGS=-pthread
CFLAGS=-std=gnu99 -pedantic -Wall -Wextra -Ilib -I.

ifndef RELEASE
CFLAGS+=-g
else
CFLAGS+=-O3 -Os
endif

all: libj conbot jbot

libj: libj.a
lib/libj.a: lib/bmap.o lib/cbuffer.o
	$(AR) rcs $@ $^

conbot: bin/conbot
bin/conbot: conbot/conbot.o conbot/ircsock.o lib/libj.a
	$(CC) -o $@ $^ $(LDFLAGS)
jbot: bin/jbot
bin/jbot: jbot/jbot.o jbot/util.o jbot/functions.o jbot/greetings.o lib/libj.a
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f lib/*.o conbot/*.o jbot/*.o
	rm -f lib/libj.a bin/conbot bin/jbot
