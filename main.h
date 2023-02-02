#ifndef __HEADER_MAIN_H__
#define __HEADER_MAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mystring.h"
#include "utils.h"
#include "parsers.h"
#include "main.h"

extern String buf;
extern String outbuf;
extern String clip;

size_t pos2idx(String *buf, char *posstr);
int load_buffer(String *buf, char *path);
void save_buffer(String *buf, char *path);
void create_file(char *path);
void cat_file(char *path);
void insert_command(char *split_cmd[], int _file, int _str, int _pos);
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

#endif	// __HEADER_MAIN_H__