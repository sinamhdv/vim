#ifndef __HEADER_MAIN_H__
#define __HEADER_MAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "mystring.h"
#include "utils.h"
#include "parsers.h"
#include "main.h"

extern String buf;
extern String outbuf;
extern String clip;
extern String pipebuf;

typedef struct
{
	size_t L;
	size_t R;
} FindAns;

#define STAR_CHAR '\x80'

size_t pos2idx(String *buf, char *posstr);
int load_buffer(String *buf, char *path);
void save_buffer(String *buf, char *path);
void create_file(char *path);
void cat_file(char *path);
void insert_command(char *filename, char *posstr, char *str, size_t slen);
void selection_action(char *split_cmd[], int _file, int _pos, int _size, int _fw, void (*action)(size_t, size_t));
void removestr(size_t L, size_t R);
void copystr(size_t L, size_t R);
void cutstr(size_t L, size_t R);
void pastestr(size_t idx);
void paste_command(char *split_cmd[], int _file, int _pos);
void compare(char *file1, char *file2);
void undo(char *path);
void create_backup(char *path);
int auto_indent_buf(String *buf);
void auto_indent_file(char *path);
void tree(char *path, String *indent_stack, size_t max_depth, int isfile);
size_t grep_file(char *root_path, char *pattern, int print_lines);
void grep_command(char *split_cmd[], int _file1, int _filen, int _c, int _l, char *str);
void find_command(char *filename, int _count, int _has_at, int _at, int _all, int _byword, char *pat);

#endif	// __HEADER_MAIN_H__