CC       = gcc
CFLAGS   = -Wall -Wextra -g -I../shared
PROG     = tetris_server

SRCS     = main.c \
           server.c \
           connection.c \
           client_manager.c \
           message_handler.c \
           server_manager.c

OBJS     = $(SRCS:.c=.o)

.PHONY: all clean

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(PROG)
