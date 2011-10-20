LDFLAGS=-pthread
CFLAGS=-pedantic -Wall -Wextra

ifndef RELEASE
CFLAGS+=-g
else
CFLAGS+=-O3 -Os
endif

all: conbot jbot

conbot: conbot.o cbuffer.o ircsock.o
	$(CC) -o $@ $^ $(LDFLAGS)
jbot: jbot.o cbuffer.o bmap.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o conbot jbot
