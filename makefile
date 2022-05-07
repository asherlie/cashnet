CC=gcc
CFLAGS= -Wall -Wextra -Wpedantic -Werror -Wshadow -Wformat=2 -fno-common -g3 -pthread -lcurses

all: nc

kq.o: kq.c kq.h

nc: kq.o


.PHONY:
clean:
	rm -f nc *.o
