CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c23 -I. -I../shared -g
LDFLAGS = -lncurses
PROG_NAME = tetris_client
ALL_OBJECTS = main.o board.o tetromino_shapes.o SRS_rotation.o textbox.o menu_maker.o difficulty_manager.o net.o score.o kstring.o keyboard_manager.o

all: $(PROG_NAME)

$(PROG_NAME): $(ALL_OBJECTS)
	$(CC) $(ALL_OBJECTS) -o $(PROG_NAME) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Specific rules if paths differ
main.o: main.c tetris/board.h net/net.h
	$(CC) $(CFLAGS) -c main.c -o main.o

board.o: tetris/board.c tetris/board.h
	$(CC) $(CFLAGS) -c tetris/board.c -o board.o

tetromino_shapes.o: tetris/tetromino_shapes.c tetris/tetromino_shapes.h
	$(CC) $(CFLAGS) -c tetris/tetromino_shapes.c -o tetromino_shapes.o

SRS_rotation.o: tetris/SRS_rotation.c tetris/SRS_rotation.h
	$(CC) $(CFLAGS) -c tetris/SRS_rotation.c -o SRS_rotation.o

textbox.o: menus/textbox.c menus/textbox.h
	$(CC) $(CFLAGS) -c menus/textbox.c -o textbox.o

menu_maker.o: menus/menu_maker.c menus/menu_maker.h
	$(CC) $(CFLAGS) -c menus/menu_maker.c -o menu_maker.o

difficulty_manager.o: tetris/difficulty_manager.c tetris/difficulty_manager.h
	$(CC) $(CFLAGS) -c tetris/difficulty_manager.c -o difficulty_manager.o

net.o: net/net.c net/net.h
	$(CC) $(CFLAGS) -c net/net.c -o net.o

score.o: tetris/score.c tetris/score.h
	$(CC) $(CFLAGS) -c tetris/score.c -o score.o

kstring.o: ../shared/kstring.c ../shared/kstring.h
	$(CC) $(CFLAGS) -c ../shared/kstring.c -o kstring.o

keyboard_manager.o: menus/keyboard_manager.c menus/keyboard_manager.h
	$(CC) $(CFLAGS) -c menus/keyboard_manager.c -o keyboard_manager.o

clean:
	rm -f *.o $(PROG_NAME)
