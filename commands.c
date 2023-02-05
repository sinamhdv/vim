#include "commands.h"

String buf;		// the editor's buffer
String inpbuf;	// command input buffer
String outbuf;	// command output buffer
String clip;	// clipboard
String pipebuf;	// pipes buffer



int load_buffer(String *buf, char *path)
{
	if (address_error(path)) return -1;
	if (file_not_found_error(path)) return -1;
	FILE *fp = fopen(path, "r");
	size_t size = get_file_size(fp);
	free(buf->arr);
	buf->arr = malloc(size * sizeof(char));
	buf->cap = buf->len = size;
	fread(buf->arr, sizeof(char), size, fp);
	fclose(fp);
	return 0;
}

void save_buffer(String *buf, char *path)
{
	create_backup(path);
	FILE *fp = fopen(path, "w");
	fwrite(buf->arr, sizeof(char), buf->len, fp);
	fclose(fp);
}

void create_file(char *path)
{
	for (int i = 0; path[i] != '\0'; i++)
	{
		if (path[i] == '/' && i != 0)
		{
			path[i] = 0;
			mkdir(path, 0775);
			path[i] = '/';
		}
	}
	
	// check if the file already exists
	if (access(path, F_OK) == 0)
	{
		print_msg("Error: File already exists");
		return;
	}

	FILE *fp = fopen(path, "w");
	if (fp == NULL)
	{
		print_msg("Error: Cannot create the file");
		return;
	}
	fclose(fp);
}

void cat_file(char *path)
{
	//create_backup(path);	// TODO: should cat be undoable at all?
	if (address_error(path)) return;
	if (file_not_found_error(path)) return;
	FILE *fp = fopen(path, "r");
	if (fp == NULL)
	{
		print_msg("Error: could not open file");
		return;
	}
	int c;
	while ((c = fgetc(fp)) != EOF) string_pushc(&outbuf, c);
	fclose(fp);
}

void insert_command(char *filename, char *posstr, char *str, size_t slen)
{
	if (filename == NULL)	// phase2 version
	{
		size_t idx = posstr2idx(&buf, posstr);
		if (idx == -1)
		{
			print_msg("Error: Invalid position");
		}
		else
		{
			string_insert(&buf, str, slen, idx);
			cursor_idx = idx + slen;
			refresh_buffer_vars(&buf);
			is_saved = 0;
		}
		return;
	}

	char *path = convert_path(filename);
	if (load_buffer(&inpbuf, path) != -1)
	{
		size_t idx = posstr2idx(&inpbuf, posstr);
		if (idx == -1)
		{
			print_msg("Error: Invalid position");
		}
		else
		{
			string_insert(&inpbuf, str, slen, idx);
			save_buffer(&inpbuf, path);
		}
	}
	free(path);
}

void selection_action(char *split_cmd[], int _file, int _pos, int _size, int _fw, void (*action)(size_t, size_t))
{
	if (_file == 0)	// phase2 version
	{
		size_t idx = posstr2idx(&buf, split_cmd[_pos]);
		if (idx == -1)
		{
			print_msg("Error: Invalid position");
		}
		else
		{
			size_t size = strtoull(split_cmd[_size], NULL, 10);
			size_t L = idx - (!_fw ? size : 0);
			size_t R = idx + (_fw ? size : 0);
			if ((long long)L < 0 || R > buf.len)
			{
				print_msg("Error: Invalid size");
			}
			else
			{
				cursor_idx = L;
				saved_cursor = R - 1;
				if (action == removestr)
				{
					remove_selection();
					is_saved = 0;
				}
				else if (action == copystr)
				{
					copy_selection();
					refresh_buffer_vars(&buf);
				}
				else if (action == cutstr)
				{
					copy_selection();
					remove_selection();
					is_saved = 0;
				}
			}
		}
		return;
	}

	char *path = convert_path(split_cmd[_file]);
	if (load_buffer(&inpbuf, path) != -1)
	{
		size_t idx = posstr2idx(&inpbuf, split_cmd[_pos]);
		if (idx == -1)
		{
			print_msg("Error: Invalid position");
		}
		else
		{
			size_t size = strtoull(split_cmd[_size], NULL, 10);
			size_t L = idx - (!_fw ? size : 0);
			size_t R = idx + (_fw ? size : 0);
			if ((long long)L < 0 || R > inpbuf.len)
			{
				print_msg("Error: Invalid size");
			}
			else
			{
				action(L, R);
				save_buffer(&inpbuf, path);
			}
		}
	}
	free(path);
}

void removestr(size_t L, size_t R)
{
	string_remove(&inpbuf, L, R);
}

void copystr(size_t L, size_t R)
{
	string_free(&clip);
	string_init(&clip, R - L);
	memcpy(clip.arr, inpbuf.arr + L, R - L);
}

void cutstr(size_t L, size_t R)
{
	copystr(L, R);
	removestr(L, R);
}

void pastestr(size_t idx)
{
	string_insert(&inpbuf, clip.arr, clip.len, idx);
}

void paste_command(char *split_cmd[], int _file, int _pos)
{
	if (!_file)	// phase2 version
	{
		size_t idx = posstr2idx(&buf, split_cmd[_pos]);
		if (idx == -1)
		{
			print_msg("Error: Invalid position");
		}
		else
		{
			string_insert(&buf, clip.arr, clip.len, idx);
			cursor_idx = idx + clip.len;
			refresh_buffer_vars(&buf);
			is_saved = 0;
		}
		return;
	}

	char *path = convert_path(split_cmd[_file]);
	if (load_buffer(&inpbuf, path) != -1)
	{
		size_t idx = posstr2idx(&inpbuf, split_cmd[_pos]);
		if (idx == -1)
		{
			print_msg("Error: Invalid position");
		}
		else
		{
			pastestr(idx);
			save_buffer(&inpbuf, path);
		}
	}
	free(path);
}

void compare(char *file1, char *file2)
{
	if (address_error(file1)) return;
	if (address_error(file2)) return;
	if (file_not_found_error(file1)) return;
	if (file_not_found_error(file2)) return;
	FILE *fps[2];
	fps[0] = fopen(file1, "r");
	fps[1] = fopen(file2, "r");
	
	char *lines[2];
	size_t sizes[2];
	int ret[2];
	int line_no = 0;
	while (1)
	{
		line_no++;
		for (int i = 0; i < 2; i++)
		{
			lines[i] = NULL;
			sizes[i] = 0;
			ret[i] = getline(&lines[i], &sizes[i], fps[i]);
		}
		if (ret[0] == -1 || ret[1] == -1) break;
		for (int i = 0; i < 2; i++)
		{
			char *tmp = strchr(lines[i], '\n');
			if (tmp) *tmp = 0;
		}
		if (strcmp(lines[0], lines[1]) != 0)
		{
			string_printf(&outbuf, "==================== #%d ====================\n< %s\n> %s\n", line_no, lines[0], lines[1]);
		}
		free(lines[0]);
		free(lines[1]);
	}
	if (ret[0] == -1 && ret[1] == -1)
	{
		fclose(fps[0]);
		fclose(fps[1]);
		free(lines[0]);
		free(lines[1]);
		return;
	}
	int remaining_idx = (ret[1] != -1);
	for (int i = 0; i < 20; i++) string_pushc(&outbuf, remaining_idx ? '>' : '<');
	int max_lines = count_file_lines(fps[remaining_idx]);
	string_printf(&outbuf, " #%d - #%d ", line_no, max_lines);
	for (int i = 0; i < 20; i++) string_pushc(&outbuf, remaining_idx ? '>' : '<');
	string_pushc(&outbuf, '\n');

	char *tmp = strchr(lines[remaining_idx], '\n');
	if (tmp) *tmp = 0;
	string_printf(&outbuf, "%s\n", lines[remaining_idx]);

	int ch = fgetc(fps[remaining_idx]);
	while (ch != EOF)
	{
		string_pushc(&outbuf, ch);
		ch = fgetc(fps[remaining_idx]);
	}
	string_pushc(&outbuf, '\n');

	fclose(fps[0]);
	fclose(fps[1]);
	free(lines[0]);
	free(lines[1]);
}

void undo(char *path)
{
	if (address_error(path)) return;
	if (file_not_found_error(path)) return;
	
	char *backup_path = get_backup_path(path);
	//fprintf(stderr, "DBG: undofile(%s) => %s\n", path, backup_path);

	if (access(backup_path, F_OK) != 0)
	{
		print_msg("Error: Backup file not found");
		return;
	}

	size_t blen = strlen(backup_path);
	char *tmp_path = strdup(backup_path);
	tmp_path[blen - 1] = 'a';	// .backua
	copy_file(path, tmp_path);
	copy_file(backup_path, path);
	copy_file(tmp_path, backup_path);
	remove(tmp_path);
	free(tmp_path);
	free(backup_path);
}

void create_backup(char *path)
{
	if (access(path, F_OK) != 0) return;
	char *backup_path = get_backup_path(path);
	copy_file(path, backup_path);
	free(backup_path);
}

int auto_indent_buf(String *buf)
{
	// check valid bracket sequence
	int cnt = 0;
	for (size_t i = 0; i < buf->len; i++)
	{
		if (buf->arr[i] == '{') cnt++;
		else if (buf->arr[i] == '}') cnt--;
		if (cnt < 0)
		{
			print_msg("Error: Invalid bracket sequence");
			return -1;
		}
	}

	// remove whitespace before and after { and }
	size_t i = 0;
	String res;
	string_init(&res, buf->len);
	res.len = 0;
	while (i < buf->len)
	{
		if (buf->arr[i] == '{' || buf->arr[i] == '}')
		{
			while (res.len > 0 && isspace(res.arr[res.len - 1])) string_popc(&res);
			string_pushc(&res, buf->arr[i]);
			i++;
			while (i < buf->len && isspace(buf->arr[i])) i++;
		}
		else
		{
			string_pushc(&res, buf->arr[i]);
			i++;
		}
	}
	memcpy(buf->arr, res.arr, res.len);
	buf->len = res.len;

	// remove whitespace in the beginning of the lines
	i = 0;
	while (i < buf->len && isspace(buf->arr[i]) && buf->arr[i] != '\n') i++;
	res.len = 0;
	while (i < buf->len)
	{
		string_pushc(&res, buf->arr[i]);
		i++;
		if (buf->arr[i - 1] == '\n')
		{
			while (i < buf->len && isspace(buf->arr[i]) && buf->arr[i] != '\n') i++;
		}
	}
	memcpy(buf->arr, res.arr, res.len);
	buf->len = res.len;

	res.len = 0;
	int cur_tabs = 0;
	for (i = 0; i < buf->len; i++)
	{
		if (buf->arr[i] == '{')
		{
			if (i > 0 && buf->arr[i - 1] != '{' && buf->arr[i - 1] != '}')
				string_pushc(&res, ' ');
			string_pushc(&res, '{');
			string_pushc(&res, '\n');
			cur_tabs++;
			if (i + 1 < buf->len && buf->arr[i + 1] != '}')
				for (int j = 0; j < cur_tabs; j++)
					string_pushc(&res, '\t');
		}
		else if (buf->arr[i] == '}')
		{
			if (i > 0 && buf->arr[i - 1] != '{' && buf->arr[i - 1] != '}')
				string_pushc(&res, '\n');
			cur_tabs--;
			for (int j = 0; j < cur_tabs; j++)
				string_pushc(&res, '\t');
			string_pushc(&res, '}');
			string_pushc(&res, '\n');
			if (i + 1 < buf->len && buf->arr[i + 1] != '}')
				for (int j = 0; j < cur_tabs; j++)
					string_pushc(&res, '\t');
		}
		else if (buf->arr[i] == '\n')
		{
			string_pushc(&res, '\n');
			for (int j = 0; j < cur_tabs; j++)
				string_pushc(&res, '\t');
		}
		else
		{
			string_pushc(&res, buf->arr[i]);
		}
	}
	string_free(buf);
	string_init(buf, res.len);
	memcpy(buf->arr, res.arr, res.len);
	string_free(&res);

	return 0;
}

void auto_indent_file(char *path)
{
	if (load_buffer(&inpbuf, path) == -1) return;
	if (auto_indent_buf(&inpbuf) == -1) return;
	save_buffer(&inpbuf, path);
}

void tree(char *path, String *indent_stack, size_t max_depth, int isfile)
{
	if (indent_stack->len > max_depth) return;

	// print the current line
	for (int i = 0; i < (int)indent_stack->len - 1; i++)
	{
		if (indent_stack->arr[i] == '0')
			string_printf(&outbuf, "|   ");
		else
			string_printf(&outbuf, "    ");
	}
	if (indent_stack->len) string_printf(&outbuf, "|__ ");
	string_insert(&outbuf, path, strlen(path), outbuf.len);
	string_pushc(&outbuf, '\n');

	if (isfile) return;
	chdir(path);
	
	// count children
	DIR *dir = opendir(".");
	struct dirent *ent;
	size_t child_count = 0;
	long start_loc = telldir(dir);
	while ((ent = readdir(dir)) != NULL)
		if ((ent->d_type == DT_DIR || ent->d_type == DT_REG) && ent->d_name[0] != '.')
			child_count++;
	seekdir(dir, start_loc);

	// dfs on children
	while ((ent = readdir(dir)) != NULL)
	{
		if (ent->d_type != DT_DIR && ent->d_type != DT_REG) continue;
		if (ent->d_name[0] == '.') continue;
		child_count--;
		
		string_pushc(indent_stack, (child_count == 0 ? '1' : '0'));
		tree(ent->d_name, indent_stack, max_depth, ent->d_type == DT_REG);
		string_popc(indent_stack);
	}

	closedir(dir);
	chdir("..");
}

size_t grep_file(char *root_path, char *pattern, int print_lines)
{
	char *path = convert_path(root_path);
	if (address_error(path)) return 0;
	if (file_not_found_error(path)) return 0;
	
	FILE *fp = fopen(path, "r");
	size_t cnt = 0;
	char *line = NULL;
	size_t size = 0;
	while (getline(&line, &size, fp) != -1)
	{
		char *tmp = strchr(line, '\n');
		if (tmp) *tmp = 0;
		char *match = strstr(line, pattern);
		if (match != NULL)
		{
			cnt++;
			if (print_lines)
			{
				string_printf(&outbuf, "%s: %s\n", root_path, line);
			}
		}
		size = 0;
		free(line);
		line = NULL;
	}

	fclose(fp);
	free(path);
	return cnt;
}

void grep_command(char *split_cmd[], int _file1, int _filen, int _c, int _l, char *str)
{
	char *tmp = strchr(str, '\n');
	if (tmp) *tmp = 0;
	size_t match_count = 0;
	for (int i = _file1; i <= _filen; i++)
	{
		size_t matches = grep_file(split_cmd[i], str,_c == 0 && _l == 0);
		match_count += matches;
		if (_l && matches)
			string_printf(&outbuf, "%s\n", split_cmd[i]);
	}
	if (_c)
		string_printf(&outbuf, "%lu\n", match_count);
}

size_t get_word(String *buf, size_t idx)
{
	size_t ans = 0;
	for (size_t i = 1; i <= idx; i++)
		if (isspace(buf->arr[i]) && !isspace(buf->arr[i - 1]))
			ans++;
	return ans;
}

FindAns find_buf(char *str, char *pat)
{
	FindAns res = {-1, -1};
	size_t slen = strlen(str);
	size_t plen = strlen(pat);
	char *star = strchr(pat, STAR_CHAR);
	char *space = strchr(pat, ' ');
	if (space && star)	// complex wildcards
	{
		*star = 0;
		char *ptr;
		size_t begin = 0;
		while ((ptr = strstr(str + begin, pat)) != NULL)
		{

		}
	}
	else if (star == pat)	// *a
	{
		
	}
	else if (star)	// a*
	{
		*star = 0;
		char *ptr = strstr(str, pat);
		if (ptr != NULL)
		{
			res.L = ptr - str;
			res.R = ptr - str + plen - 2;
			while (!isspace(str[res.R + 1]) && str[res.R + 1] != '\0') res.R++;
		}
	}
	else	// no wildcard
	{
		char *ptr = strstr(str, pat);
		if (ptr != NULL)
		{
			res.L = ptr - str;
			res.R = ptr - str + plen - 1;
		}
	}
	return res;
}

FindAns *findall_buf(char *str, char *pat)
{
	FindAns *res = malloc(8 * sizeof(FindAns));
	size_t cap = 8;
	size_t len = 0;
	FindAns cur = {};
	size_t begin = 0;
	while ((cur = find_buf(str + begin, pat)).L != -1)
	{
		cur.L += begin;
		cur.R += begin;
		begin = cur.R + 1;
		if (len == cap)
		{
			cap *= 2;
			res = realloc(res, cap * sizeof(FindAns));
		}
		res[len++] = cur;
	}
	if (len == cap)
	{
		cap *= 2;
		res = realloc(res, cap * sizeof(FindAns));
	}
	res[len].L = res[len].R = -1;
	return res;
}

void find_command(char *filename, int _count, int _has_at, int _at, int _all, int _byword, char *pat)
{
	if (_count && (_has_at || _all || _byword)) {
		print_msg("Error: Invalid options");
		return;
	}
	else if (_has_at && _all) {
		print_msg("Error: Invalid options");
		return;
	}
	else if (_has_at && _at <= 0) {
		print_msg("Error: Invalid -at argument");
		return;
	}

	char *tmp = strchr(pat, '\n');
	if (tmp) *tmp = 0;
	parsestr_wildcard(pat);
	char *path = convert_path(filename);
	if (load_buffer(&inpbuf, path) != -1)
	{
		string_null_terminate(&inpbuf);
		FindAns *res = findall_buf(inpbuf.arr, pat);
		if (_count)
		{
			size_t size = 0;
			while (res[size].L != -1) size++;
			string_printf(&outbuf, "%lu\n", size);
		}
		else if (_all)
		{
			if (res[0].L == -1)
				print_msg("Pattern not found");
			else
			{
				for (size_t i = 0; res[i].L != -1; i++)
					string_printf(&outbuf, (res[i + 1].L != -1 ? "%lu, " : "%lu"), (_byword ? get_word(&inpbuf, res[i].L) : res[i].L));
				string_pushc(&outbuf, '\n');
			}
		}
		else
		{
			if (!_has_at) _at = 1;
			size_t size = 0;
			while (res[size].L != -1) size++;
			if (_at > size)
				print_msg("Pattern not found");
			else
				string_printf(&outbuf, "%lu\n", (_byword ? get_word(&inpbuf, res[_at - 1].L) : res[_at - 1].L));
		}
		free(res);
	}
	free(path);
}

void replace_command(char *filename, char *old, char *new, int _has_at, int _at, int _all)
{
	if (_has_at && _all) {
		print_msg("Error: Invalid options");
		return;
	}
	else if (_has_at && _at <= 0) {
		print_msg("Error: Invalid -at argument");
		return;
	}

	char *tmp = strchr(old, '\n');
	if (tmp) *tmp = 0;
	parsestr_wildcard(old);
	char *path = convert_path(filename);
	if (load_buffer(&inpbuf, path) != -1)
	{
		string_null_terminate(&inpbuf);
		FindAns *res = findall_buf(inpbuf.arr, old);
		if (_all)
		{
			if (res[0].L == -1)
				print_msg("Error: Pattern not found");
			else
			{
				size_t offset = 0;
				size_t nlen = strlen(new);
				for (size_t i = 0; res[i].L != -1; i++)
				{
					string_replace(&inpbuf, res[i].L + offset, res[i].R + offset + 1, new, nlen);
					offset += nlen - (res[i].R - res[i].L + 1);
				}
				save_buffer(&inpbuf, path);
			}
		}
		else
		{
			if (!_has_at) _at = 1;
			size_t size = 0;
			while (res[size].L != -1) size++;
			if (_at > size)
				print_msg("Pattern not found");
			else
			{
				string_replace(&inpbuf, res[_at - 1].L, res[_at - 1].R + 1, new, strlen(new));
				save_buffer(&inpbuf, path);
			}
		}
		free(res);
	}
	free(path);
}

int phase1_main(void)
{
	// initialize global strings
	string_init(&outbuf, 0);

	while (1)
	{
		char *cmd = NULL;
		size_t len = 0;
		if (getline(&cmd, &len, stdin) == -1)
		{
			free(cmd);
			break;
		}
		int ret = parse_line(cmd);
		free(cmd);
		if (ret) break;
		//if (clip.arr != NULL) fprintf(stderr, "DBG: CLIP[%lu] = '%s'\n", clip.len, clip.arr);
	}

	// free allocated memory
	string_free(&inpbuf);
	string_free(&outbuf);
	string_free(&clip);
	string_free(&pipebuf);

	return 0;
}
