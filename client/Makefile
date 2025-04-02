CC = gcc
CFLAGS = -lncurses -Wall -Wextra -I.
PROG_NAME = tetris_client

all: $(PROG_NAME)

$(PROG_NAME): main.o test.o
	$(CC) main.o test.o -o $(PROG_NAME) ${CFLAGS}

main.o: main.c test.h
	$(CC) -c main.c $(CFLAGS)

test.o: test.c test.h
	$(CC) -c test.c $(CFLAGS)

clean:
	rm -f *.o $(PROG_NAME)
