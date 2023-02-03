#ifndef __HEADER_UTILS_H__
#define __HEADER_UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ncurses.h>

#include "mystring.h"
#include "utils.h"
#include "parsers.h"
#include "main.h"
#include "commands.h"

#define ROOT_DIR "root"

extern String buf;
extern String outbuf;
extern String clip;
extern String pipebuf;

char *convert_path(char *path);
int address_error(char *path);
int file_not_found_error(char *path);
size_t get_file_size(FILE *fp);
int count_file_lines(FILE *fp);	// count the number of lines in a file without changing the cursor position
char *get_backup_path(char *path);
void copy_file(char *src, char *dst);
int count_digits(int num);

#endif	// __HEADER_UTILS_H__