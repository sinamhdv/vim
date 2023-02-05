#include "main.h"

char *mode_names[] = {"NORMAL", "INSERT", "VISUAL"};

String buf;			// the editor's buffer
String buf_backup;	// undo backup buffer
String inpbuf;		// command input buffer
String outbuf;		// command output buffer
String pipebuf;		// pipes buffer
String clip;		// clipboard

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
int cursor_backup_idx;	// undo backup cursor index
int saved_cursor;	// saved cursor index for visual mode

FindAns *find_matches;	// matches for find that should be highlighted

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
	string_free(&buf_backup);
	free(filename);
	filename = NULL;
}

// ensure that the buffer ends with '\n'
void terminate_with_LF(String *buf)
{
	if (buf->len == 0)
	{
		string_resize(buf, 1);
		buf->len = 1;
		buf->arr[0] = '\n';
	}
	else if (buf->arr[buf->len - 1] != '\n')
	{
		string_pushc(buf, '\n');
	}
}

// initialize global vars based a newly opened buffer
void init_new_buf(String *buf)
{
	terminate_with_LF(buf);
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
	cursor_backup_idx = -1;
}

// refresh buffer global vars based on cursor_idx and previous top_line/bottom_line
void refresh_buffer_vars(String *buf)
{
	terminate_with_LF(buf);
	total_lines = 0;
	for (size_t i = 0; i < buf->len; i++)
	{
		if (i == cursor_idx) cursor_line = total_lines + 1;
		total_lines += (buf->arr[i] == '\n');
	}
	cursor_calc_lpos();

	// cut adjustments
	if (bottom_line > total_lines)
	{
		bottom_line = total_lines;
	}
	if (cursor_line < top_line)
	{
		top_line = cursor_line;
		bottom_line = top_line + SCR_HEIGHT - 3;
		if (bottom_line > total_lines) bottom_line = total_lines;
	}

	// paste adjustments
	if (bottom_line - top_line < SCR_HEIGHT - 3 && total_lines > bottom_line)
	{
		bottom_line = total_lines;
		if (bottom_line - top_line > SCR_HEIGHT - 3)
			top_line = bottom_line - (SCR_HEIGHT - 3);
	}
	if (cursor_line > bottom_line)
	{
		bottom_line = cursor_line;
		top_line = bottom_line - (SCR_HEIGHT - 3);
		if (top_line < 1) top_line = 1;
	}
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

void print_status_bar(void)
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
	for (; x < SCR_WIDTH - 30; x++)
		addch(' ');
	printw("%d:%d - %d", cursor_line, cursor_lpos, cursor_idx);
	getyx(stdscr, y, x);
	for (; x < SCR_WIDTH; x++)
		addch(' ');
	attroff(COLOR_PAIR(FILENAME_COLOR));
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
			// selection highlighting
			int highlight = 0;
			int L = cursor_idx;
			int R = saved_cursor;
			if (R < L)
			{
				int tmp = R;
				R = L;
				L = tmp;
			}
			if (mode == VISUAL && ptr - buf->arr >= L && ptr - buf->arr <= R) highlight = 1;
			if (mode == NORMAL && find_matches != NULL)	// find highlighting
			{
				for (int i = 0; find_matches[i].L != -1; i++)
				{
					if (ptr - buf->arr >= find_matches[i].L && ptr - buf->arr <= find_matches[i].R)
					{
						highlight = 1;
						break;
					}
				}
			}
			if (highlight) attron(COLOR_PAIR(SELECTION_COLOR));

			if (*ptr == '\t')
			{
				int y, x;
				getyx(stdscr, y, x);
				x -= lineno_width;
				for (int i = 0; i < TABSIZE - x % TABSIZE; i++)
					addch(' ');
			}
			else if (*ptr == '{' || *ptr == '}')
			{
				if (!highlight) attron(COLOR_PAIR(CRBRACE_COLOR));
				addch(*ptr);
				if (!highlight) attroff(COLOR_PAIR(CRBRACE_COLOR));
			}
			else if (*ptr == '(' || *ptr == ')')
			{
				if (!highlight) attron(COLOR_PAIR(BRACE_COLOR));
				addch(*ptr);
				if (!highlight) attroff(COLOR_PAIR(BRACE_COLOR));
			}
			else if (*ptr == '[' || *ptr == ']')
			{
				if (!highlight) attron(COLOR_PAIR(SQBRACE_COLOR));
				addch(*ptr);
				if (!highlight) attroff(COLOR_PAIR(SQBRACE_COLOR));
			}
			else
				addch(*ptr);
			ptr++;

			if (highlight) attroff(COLOR_PAIR(SELECTION_COLOR));
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
	print_status_bar();
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
	else
	{
		graphical_find(cmd);
	}
}

void copy_selection(void)
{
	size_t L = saved_cursor;
	size_t R = cursor_idx;
	if (R < L)
	{
		size_t tmp = R;
		R = L;
		L = tmp;
	}
	string_free(&clip);
	string_init(&clip, R - L + 1);
	memcpy(clip.arr, buf.arr + L, R - L + 1);
}

void remove_selection(void)
{
	size_t L = saved_cursor;
	size_t R = cursor_idx;
	if (R < L)
	{
		size_t tmp = R;
		R = L;
		L = tmp;
	}
	if (L == buf.len - 1) return;
	if (R == buf.len - 1) R--;
	string_remove(&buf, L, R + 1);
	cursor_idx = L;
	refresh_buffer_vars(&buf);
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

void cursor_left(void)
{
	int dist = cursor_idx;
	cursor_home();
	dist -= cursor_idx;
	for (int i = 0; i < dist - 1; i++) cursor_right();
}

// use this to calculate cursor_lpos correctly when cursor_idx is correct
void cursor_calc_lpos(void)
{
	if (cursor_idx > 0 && buf.arr[cursor_idx - 1] != '\n')
	{
		cursor_left();
		cursor_right();
	}
	else
		cursor_left();
}

void create_backup_buf(void)
{
	string_free(&buf_backup);
	string_init(&buf_backup, buf.len);
	memcpy(buf_backup.arr, buf.arr, buf.len);
	cursor_backup_idx = cursor_idx;
}

void undo_buf(void)
{
	if (cursor_backup_idx == -1)
	{
		print_msg("Error: No change to undo");
		return;
	}
	String tmp = buf;
	buf = buf_backup;
	buf_backup = tmp;
	int ctmp = cursor_idx;
	cursor_idx = cursor_backup_idx;
	cursor_backup_idx = ctmp;
	refresh_buffer_vars(&buf);
	is_saved = 0;
}

void graphical_find(char *pat)
{
	char *tmp = strchr(pat, '\n');
	if (tmp) *tmp = 0;
	if (pat[0] == '\0')
	{
		print_msg("Error: empty pattern");
		return;
	}
	parsestr_wildcard(pat);
	string_null_terminate(&buf);
	find_matches = findall_buf(buf.arr, pat);
	if (find_matches[0].L == -1)
	{
		print_msg("Pattern not found");
		free(find_matches);
		find_matches = NULL;
		return;
	}
	int cur_match = 0;
	cursor_idx = find_matches[0].L;
	refresh_buffer_vars(&buf);
	draw_window();
	while (1)
	{
		int ch = getch();
		if (ch == 'n')
		{
			cur_match++;
			if (find_matches[cur_match].L == -1) cur_match = 0;
			cursor_idx = find_matches[cur_match].L;
			refresh_buffer_vars(&buf);
			draw_window();
		}
		else
		{
			free(find_matches);
			find_matches = NULL;
			break;
		}
	}
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
			if (bottom_line - top_line > SCR_HEIGHT - 3)
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
		cursor_left();
	}
	
	// insert mode
	else if (mode == NORMAL && key == 'i')
	{
		mode = INSERT;
		create_backup_buf();
	}
	else if (mode == INSERT && key == KEY_ESC)
	{
		mode = NORMAL;
	}
	else if (mode == INSERT && key == KEY_BACKSPACE)
	{
		if (cursor_idx == 0) return 0;
		is_saved = 0;
		if (buf.arr[cursor_idx - 1] != '\n')
		{
			string_remove(&buf, cursor_idx - 1, cursor_idx);
			cursor_idx--;
			cursor_calc_lpos();
		}
		else
		{
			string_remove(&buf, cursor_idx - 1, cursor_idx);
			cursor_line--;
			total_lines--;
			cursor_idx--;
			cursor_calc_lpos();
			if (cursor_line < top_line)	// backspace in the first line
			{
				top_line--;
				bottom_line--;
			}
			else if (bottom_line > total_lines)
				bottom_line--;
		}
	}
	//else if (mode == INSERT && key == KEY_DC) {}	// possible implementation of DEL key
	else if (mode == INSERT && (isprint(key) || key == '\t'))
	{
		is_saved = 0;
		string_insert(&buf, (char *)&key, 1, cursor_idx);
		cursor_right();
	}
	else if (mode == INSERT && key == '\n')
	{
		is_saved = 0;
		string_insert(&buf, (char *)&key, 1, cursor_idx);
		cursor_idx++;
		cursor_line++;
		cursor_lpos = 0;
		total_lines++;

		if (cursor_line > bottom_line)	// enter in the last line
		{
			bottom_line++;
			if (bottom_line - top_line > SCR_HEIGHT - 3)
				top_line++;
		}
		else if (bottom_line - top_line < SCR_HEIGHT - 3)
			bottom_line++;
	}
	
	// visual mode
	else if (mode == NORMAL && key == 'v')
	{
		mode = VISUAL;
		saved_cursor = cursor_idx;
	}
	else if (mode == VISUAL && (key == KEY_ESC || key == 'v'))
	{
		mode = NORMAL;
	}
	else if (mode == VISUAL && key == 'd')	// cut
	{
		mode = NORMAL;
		create_backup_buf();
		copy_selection();
		remove_selection();		
		is_saved = 0;
	}
	else if (mode == VISUAL && key == 'y')	// copy
	{
		mode = NORMAL;
		copy_selection();
	}

	else if (mode == NORMAL && key == '=')	// auto-indent
	{
		create_backup_buf();
		auto_indent_buf(&buf);
		cursor_idx = 0;
		refresh_buffer_vars(&buf);
		is_saved = 0;
	}

	else if (mode == NORMAL && key == 'p')	// paste
	{
		create_backup_buf();
		string_insert(&buf, clip.arr, clip.len, cursor_idx);
		cursor_idx += clip.len;
		refresh_buffer_vars(&buf);
		is_saved = 0;
	}

	else if (mode == NORMAL && key == 'u')	// undo
	{
		undo_buf();
	}

	return 0;
}

void sigint_handler(int signum)
{
	string_free(&buf);
	string_free(&buf_backup);
	string_free(&inpbuf);
	string_free(&outbuf);
	string_free(&pipebuf);
	string_free(&clip);
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
	set_escdelay(0);	// cancel the delay for ESC key
	struct sigaction sigint_action = {{sigint_handler}};
	sigaction(SIGINT, &sigint_action, NULL);

	// init colors
	init_pair(MODE_COLOR, COLOR_BLACK, COLOR_GREEN);
	init_pair(FILENAME_COLOR, COLOR_WHITE, COLOR_BLUE);
	init_pair(LINENO_COLOR, COLOR_WHITE, COLOR_BLUE);
	init_pair(ERROR_COLOR, COLOR_RED, COLOR_BLACK);
	init_pair(BRACE_COLOR, COLOR_GREEN, COLOR_BLACK);
	init_pair(SQBRACE_COLOR, COLOR_RED, COLOR_BLACK);
	init_pair(CRBRACE_COLOR, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(SELECTION_COLOR, COLOR_BLACK, COLOR_CYAN);

	// init editor
	string_init(&buf, 0);
	init_new_buf(&buf);
	is_saved = 1;

	while (1)
	{
		draw_window();
		input_loop();
	}

	sigint_handler(0);
	return 0;
}