#include "main.h"

char *mode_names[] = {"NORMAL", "INSERT", "VISUAL"};

Mode mode = NORMAL;		// current editor mode
char *filename = NULL;	// current filename
int is_saved = 1;	// is the current buffer saved?

int total_lines = 1;	// total lines of the current buffer
int top_line = 1;		// number of the top visible line
int bottom_line = 1;		// number of the bottom visible line
int lineno_width;	// the width of the line numbers

int cursor_line = 1;	// cursor line number
int cursor_lpos = 0;	// cursor position in line on screen
int cursor_idx = 0;		// cursor index in buffer

void clearline(int num)
{
	move(num, 0);
	for (int i = 0; i < SCR_WIDTH; i++) addch(' ');
}

int saveas_file(char *path)
{
	if (address_error(path)) return 0;
	save_buffer(&buf, path);
	if (filename == NULL) filename = strdup(path);
	is_saved = 1;
	return 1;
}

void save_file(void)
{
	if (filename != NULL)
		saveas_file(filename);
	while (filename == NULL)
	{
		clearline(SCR_HEIGHT - 1);
		move(SCR_HEIGHT - 1, 0);
		printw("Enter a name for the current file: ");
		char name[MAX_FILENAME];
		echo();
		getstr(name);
		noecho();
		char *path = convert_path(name);
		saveas_file(path);
		free(path);
	}
}

char get_prompt(char *msg)
{
	clearline(SCR_HEIGHT - 1);
	move(SCR_HEIGHT - 1, 0);
	printw("%s", msg);
	refresh();
	char ch = getch();
	clearline(SCR_HEIGHT - 1);
	refresh();
	return ch;
}

// close the current file in the editor
void close_file(void)
{
	if (!is_saved)
	{
		char ch = get_prompt("Do you want to save the current file? (y/n)");
		if (ch != 'N' && ch != 'n') save_file();
	}
	string_free(&buf);
	free(filename);
	filename = NULL;
}

// initialize global vars based a newly opened buffer
void init_new_buf(String *buf)
{
	// ensure that the buffer ends with '\n'
	if (buf->len == 0)
	{
		string_resize(buf, 1);
		buf->len = 1;
		buf->arr[0] = '\n';
	}
	else if (buf->arr[buf->len - 1] != '\n')
	{
		buf->arr[buf->len - 1] = '\n';
	}

	is_saved = (filename != NULL);
	total_lines = 0;
	for (size_t i = 0; i < buf->len; i++)
		total_lines += (buf->arr[i] == '\n');
	top_line = 1;
	bottom_line = SCR_HEIGHT - 2;
	if (total_lines < bottom_line) bottom_line = total_lines;

	cursor_line = 1;
	cursor_lpos = 0;
	cursor_idx = 0;
}

void open_file(char *path)
{
	if (address_error(path)) return;
	if (file_not_found_error(path)) return;
	close_file();
	filename = strdup(path);
	load_buffer(&buf, path);
	init_new_buf(&buf);
}

void print_msg(char *msg)
{
	clearline(SCR_HEIGHT - 1);
	move(SCR_HEIGHT - 1, 0);
	attron(COLOR_PAIR(ERROR_COLOR));
	printw("%s", msg);
	attroff(COLOR_PAIR(ERROR_COLOR));
	refresh();
	getch();
	clearline(SCR_HEIGHT - 1);
	refresh();
}

void print_mode_filename(void)
{
	move(SCR_HEIGHT - 2, 0);
	attron(COLOR_PAIR(MODE_COLOR));
	printw("  %s  ", mode_names[mode]);
	attroff(COLOR_PAIR(MODE_COLOR));
	attron(COLOR_PAIR(FILENAME_COLOR));
	if (filename != NULL)
		printw("  %s  ", strrchr(filename, '/') + 1);
	else
		printw("  untitled  ");
	if (!is_saved) addch('+');
	int y, x;
	getyx(stdscr, y, x);
	for (; x < SCR_WIDTH; x++)
		addch(' ');
	attroff(COLOR_PAIR(FILENAME_COLOR));
}

void print_line_numbers(void)
{
	lineno_width = count_digits(total_lines) + 1;
	attron(COLOR_PAIR(LINENO_COLOR));
	for (int i = 0; i < SCR_HEIGHT - 2 && i <= bottom_line - top_line; i++)
	{
		move(i, 0);
		printw("%*d ", lineno_width - 1, top_line + i);
	}
	attroff(COLOR_PAIR(LINENO_COLOR));
}

void print_buffer(String *buf)
{
	lineno_width = count_digits(total_lines) + 1;
	char *ptr = buf->arr + pos2idx(buf, top_line, 0);
	for (int i = 0; i < SCR_HEIGHT - 2 && i <= bottom_line - top_line; i++)
	{
		attron(COLOR_PAIR(LINENO_COLOR));
		move(i, 0);
		printw("%*d ", lineno_width - 1, top_line + i);
		attroff(COLOR_PAIR(LINENO_COLOR));
		while (*ptr != '\n')
		{
			if (*ptr == '\t')
			{
				int y, x;
				getyx(stdscr, y, x);
				x -= lineno_width;
				for (int i = 0; i < TABSIZE - x % TABSIZE; i++)
					addch(' ');
			}
			else
				addch(*ptr);
			ptr++;
		}
		addch(*ptr++);
	}
}

// move the cursor to its correct location on screen
void position_cursor(void)
{
	move(cursor_line - top_line, cursor_lpos + lineno_width);
}

void draw_window(void)
{
	clear();
	print_mode_filename();
	print_buffer(&buf);
	position_cursor();
	refresh();
}

void command_mode(char start_char)
{
	move(SCR_HEIGHT - 1, 0);
	addch(start_char);
	char cmd[MAX_CMD_LEN];
	echo();
	getstr(cmd);
	noecho();

	if (start_char == ':')
	{
		int clen = strlen(cmd);
		cmd[clen++] = '\n';
		cmd[clen] = '\0';
		int ret = parse_line(cmd);
		if (ret == 0 && outbuf.len)
		{
			// move command output into editor buffer
			close_file();
			buf = outbuf;
			outbuf.arr = NULL;
			outbuf.len = outbuf.cap = 0;
			init_new_buf(&buf);
		}
		else if (ret == 1)	// exit
		{
			close_file();
			sigint_handler(0);
		}
	}
}

int cursor_right(void)
{
	if (buf.arr[cursor_idx] == '\n') return 0;
	cursor_lpos += (buf.arr[cursor_idx] == '\t' ? TABSIZE - cursor_lpos % TABSIZE : 1);
	cursor_idx++;
	return 1;
}

// go to the beginning og the current line
void cursor_home(void)
{
	cursor_lpos = 0;
	while (cursor_idx > 0 && buf.arr[cursor_idx - 1] != '\n') cursor_idx--;
}

int input_loop(void)
{
	int key = getch();
	if (mode == NORMAL && (key == ':' || key == '/'))
	{
		command_mode(key);
	}

	// cursor navigation
	else if (key == KEY_UP || (mode != INSERT && key == 'k'))
	{
		if (cursor_line == 1) return 0;
		cursor_line--;
		if (cursor_line < top_line + 3 && top_line != 1)	// shift lines
		{
			top_line--;
			bottom_line--;
		}
		
		int tmp = cursor_lpos;
		cursor_home();
		cursor_idx--;
		cursor_home();
		while (cursor_lpos < tmp)
			if (!cursor_right())
				break;
	}
	else if (key == KEY_DOWN || (mode != INSERT && key == 'j'))
	{
		if (cursor_line == total_lines) return 0;
		cursor_line++;
		if (cursor_line > bottom_line - 3 && bottom_line != total_lines)	// shift lines
		{
			top_line++;
			bottom_line++;
		}

		int tmp = cursor_lpos;
		cursor_lpos = 0;
		while (buf.arr[cursor_idx] != '\n') cursor_idx++;
		cursor_idx++;
		while (cursor_lpos < tmp)
			if (!cursor_right())
				break;
	}
	else if (key == KEY_RIGHT || (mode != INSERT && key == 'l'))
	{
		cursor_right();
	}
	else if (key == KEY_LEFT || (mode != INSERT && key == 'h'))
	{
		int dist = cursor_idx;
		cursor_home();
		dist -= cursor_idx;
		for (int i = 0; i < dist - 1; i++) cursor_right();
	}
	return 0;
}

void sigint_handler(int signum)
{
	string_free(&buf);
	string_free(&inpbuf);
	string_free(&outbuf);
	string_free(&clip);
	string_free(&pipebuf);
	endwin();
	exit(0);
}

int main(void)
{
	initscr();	// initialize ncurses
	cbreak();		// disable line buffering
	keypad(stdscr, TRUE);	// enable getting input from arrow keys, F1, F2, etc.
	noecho();	// disable echo when getting keyboard input
	start_color();	// start using ncurses colors
	struct sigaction sigint_action = {{sigint_handler}};
	sigaction(SIGINT, &sigint_action, NULL);

	// init colors
	init_pair(MODE_COLOR, COLOR_BLACK, COLOR_GREEN);
	init_pair(FILENAME_COLOR, COLOR_WHITE, COLOR_BLUE);
	init_pair(LINENO_COLOR, COLOR_WHITE, COLOR_BLUE);
	init_pair(ERROR_COLOR, COLOR_RED, COLOR_BLACK);

	// init editor
	string_init(&buf, 0);
	init_new_buf(&buf);

	while (1)
	{
		draw_window();
		input_loop();
	}

	sigint_handler(0);
	return 0;
}