CC = gcc
CFLAGS = -lncurses -Wall -Wextra -I. -I../shared -g 
PROG_NAME = tetris_client
ALL_OBJECTS = main.o board.o tetromino_shapes.o SRS_rotation.o textbox.o menu_maker.o difficulty_manager.o net.o score.o kstring.o

all: $(PROG_NAME)

$(PROG_NAME): $(ALL_OBJECTS)
	$(CC) $(ALL_OBJECTS) -o $(PROG_NAME) $(CFLAGS)

main.o: main.c tetris/board.h net/net.h
	$(CC) -c main.c $(CFLAGS)

board.o: tetris/board.c tetris/board.h
	$(CC) -c tetris/board.c $(CFLAGS)

tetromino_shapes.o: tetris/tetromino_shapes.c tetris/tetromino_shapes.h
	$(CC) -c tetris/tetromino_shapes.c $(CFLAGS)

SRS_rotation.o: tetris/SRS_rotation.c tetris/SRS_rotation.h
	$(CC) -c tetris/SRS_rotation.c $(CFLAGS)

textbox.o: menus/textbox.c menus/textbox.h
	$(CC) -c menus/textbox.c $(CFLAGS)

menu_maker.o: menus/menu_maker.c menus/menu_maker.h
	$(CC) -c menus/menu_maker.c $(CFLAGS)

difficulty_manager.o: tetris/difficulty_manager.c tetris/difficulty_manager.h
	$(CC) -c tetris/difficulty_manager.c $(CFLAGS)

net.o: net/net.c net/net.h
	$(CC) -c net/net.c $(CFLAGS)

score.o: tetris/score.c tetris/score.h
	$(CC) -c tetris/score.c $(CFLAGS)

kstring.o: ../shared/kstring.c ../shared/kstring.h
	$(CC) -c ../shared/kstring.c $(CFLAGS)

clean:
	rm -f *.o $(PROG_NAME)
