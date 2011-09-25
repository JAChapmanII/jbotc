LDFLAGS=-pthread
CFLAGS=-pedantic -Wall -Wextra

ifndef RELEASE
CFLAGS+=-g
else
CFLAGS+=-O3 -Os
endif

jbot: jbot.o cbuffer.o ircsock.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o jbot
