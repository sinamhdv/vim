#ifndef __HEADER_PARSERS_H__
#define __HEADER_PARSERS_H__

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

#define MAX_TOKENS 1024

extern String buf;
extern String outbuf;
extern String clip;
extern String pipebuf;

char *skip_space(char *cmd);

/* read a token from the input line (with/without quotes)
returns a pointer to the end of the token */
char *readtok(char *cmd);

/* split the input line into tokens. The last token is NULL. '\' is handled in tokens */
int split(char *cmd, char *res[]);

/* parse the find pattern with wildcards */
void parsestr_wildcard(char *cmd);

/* parse a single extracted token without considering * as a special character */
void parsestr(char *cmd);

/* parse a single command (not including pipes) */
int parse_command(char *split_cmd[], int pipe_mode);

/* parse one line of input and handle pipes between commands */
int parse_line(char *cmd);

#endif	// __HEADER_PARSERS_H__