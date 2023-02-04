#ifndef __HEADER_MAIN_H__
#define __HEADER_MAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <ncurses.h>

#include "mystring.h"
#include "parsers.h"
#include "commands.h"
#include "utils.h"

extern String buf;
extern String inpbuf;
extern String outbuf;
extern String clip;
extern String pipebuf;

#define SCR_HEIGHT 55
#define SCR_WIDTH 203

#define TABSIZE 4
#define KEY_ESC 27

#define MODE_COLOR 1
#define FILENAME_COLOR 2
#define LINENO_COLOR 3
#define ERROR_COLOR 4
#define BRACE_COLOR 5
#define SQBRACE_COLOR 6
#define CRBRACE_COLOR 7
#define SELECTION_COLOR 8

#define MAX_CMD_LEN 1024
#define MAX_FILENAME 1024

typedef enum
{
	NORMAL = 0,
	INSERT = 1,
	VISUAL = 2
} Mode;

void print_msg(char *msg);
void clearline(int num);
int saveas_file(char *rtpath);
void save_file(void);
char get_prompt(char *msg);
void close_file(void);
void init_new_buf(String *buf);
void open_file(char *path);
void print_msg(char *msg);
void print_mode_filename(void);
void print_line_numbers(void);
void print_buffer(String *buf);
void draw_window(void);
void command_mode(char start_char);
int input_loop(void);
void sigint_handler(int signum);
void cursor_calc_lpos(void);
void remove_selection(void);
void copy_selection(void);
void refresh_buffer_vars(String *buf);

#endif	// __HEADER_MAIN_H__