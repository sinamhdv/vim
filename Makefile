CC = gcc
CFLAGS = -g -Wall

all:
	$(CC) $(CFLAGS) main.c commands.c parsers.c utils.c mystring.c -lncurses

