CC = gcc
CFLAGS = -g -Wall

all:
	$(CC) $(CFLAGS) main.c parsers.c utils.c mystring.c

