CC = gcc
CFLAGS = -lncurses -Wall -Wextra -I.
PROG_NAME = tetris_client

all: $(PROG_NAME)

$(PROG_NAME): main.o board.o tetromino_shapes.o
	$(CC) main.o board.o tetromino_shapes.o -o $(PROG_NAME) ${CFLAGS}

main.o: main.c tetris/board.h
	$(CC) -c main.c $(CFLAGS)

board.o: tetris/board.c tetris/board.h
	$(CC) -c tetris/board.c $(CFLAGS)

tetromino_shapes.o: tetris/tetromino_shapes.c tetris/tetromino_shapes.h
	$(CC) -c tetris/tetromino_shapes.c $(CFLAGS)

clean:
	rm -f *.o $(PROG_NAME)
