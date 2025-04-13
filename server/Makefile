CC = gcc
CFLAGS = -Wall -Wextra -I../shared -g
PROG_NAME = tetris_server
OBJS = main.o

all: $(PROG_NAME)

$(PROG_NAME): $(OBJS)
	$(CC) $(OBJS) -o $(PROG_NAME) $(CFLAGS)

main.o: main.c ../shared/protocol.h
	$(CC) -c main.c $(CFLAGS)

clean:
	rm -f *.o $(PROG_NAME)
