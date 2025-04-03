CC = gcc
CFLAGS = -lncurses -Wall -Wextra -I.
PROG_NAME = tetris_client

all: $(PROG_NAME)

$(PROG_NAME): main.o tetris_board.o tetromino_shapes.o
	$(CC) main.o tetris_board.o tetromino_shapes.o -o $(PROG_NAME) ${CFLAGS}

main.o: main.c tetris_board.h
	$(CC) -c main.c $(CFLAGS)

tetris_board.o: tetris_board.c tetris_board.h
	$(CC) -c tetris_board.c $(CFLAGS)

tetromino_shapes.o: tetromino_shapes.c tetromino_shapes.h
	$(CC) -c tetromino_shapes.c $(CFLAGS)

clean:
	rm -f *.o $(PROG_NAME)
