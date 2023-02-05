#include "parsers.h"

char *skip_space(char *cmd)
{
	while (isspace(*cmd)) cmd++;
	return cmd;
}

char *readtok(char *cmd)
{
	if (*cmd == '"')
	{
		cmd++;
		while (1)
		{
			if (*cmd == '\\') cmd += 2;
			else if (*cmd == '"') break;
			else cmd++;
		}
	}
	else
	{
		while (1)
		{
			if (*cmd == '\\') cmd += 2;
			else if (isspace(*cmd)) break;
			else cmd++;
		}
	}
	*cmd = 0;
	return cmd + 1;
}

int split(char *cmd, char *res[])
{
	int size = 0;
	while (1)
	{
		cmd = skip_space(cmd);
		if (*cmd == 0) break;
		char *end = readtok(cmd);
		if (*cmd == '"') cmd++;
		res[size] = malloc(end - cmd);
		strcpy(res[size], cmd);
		cmd = end;
		size++;
	}
	res[size] = NULL;
	return size;
}

void parsestr(char *cmd)
{
	char *ptr = cmd;
	size_t slen = strlen(cmd);
	for (size_t i = 0; i < slen; i++)
	{
		if (cmd[i] == '\\')
		{
			if (cmd[i + 1] == 'n') {
				*ptr++ = '\n';
				i++;
			}
			else if (cmd[i + 1] == '\\') {
				*ptr++ = '\\';
				i++;
			}
			else if (cmd[i + 1] == '"') {
				*ptr++ = '"';
				i++;
			}
			else {
				*ptr++ = cmd[i];
			}
		}
		else
		{
			*ptr++ = cmd[i];
		}
	}
	*ptr = 0;
}

void parsestr_wildcard(char *cmd)
{
	char *ptr = cmd;
	size_t slen = strlen(cmd);
	for (size_t i = 0; i < slen; i++)
	{
		if (cmd[i] == '\\')
		{
			if (cmd[i + 1] == 'n') {
				*ptr++ = '\n';
				i++;
			}
			else if (cmd[i + 1] == '\\') {
				*ptr++ = '\\';
				i++;
			}
			else if (cmd[i + 1] == '"') {
				*ptr++ = '"';
				i++;
			}
			else if (cmd[i + 1] == '*') {
				*ptr++ = '*';
				i++;
			}
			else {
				*ptr++ = cmd[i];
			}
		}
		else if (cmd[i] == '*')
		{
			*ptr++ = STAR_CHAR;
		}
		else
		{
			*ptr++ = cmd[i];
		}
	}
	*ptr = 0;
}

int parse_command(char *split_cmd[], int pipe_mode)
{
	if (pipe_mode)
	{
		string_init(&pipebuf, outbuf.len);
		memcpy(pipebuf.arr, outbuf.arr, outbuf.len);
	}
	string_free(&outbuf);
	string_init(&outbuf, 0);

	if (strcmp(split_cmd[0], "create") == 0)
	{
		if (pipe_mode) return -2;
		parsestr(split_cmd[2]);
		char *path = convert_path(split_cmd[2]);
		create_file(path);
		free(path);
		return 3;
	}

	else if (strcmp(split_cmd[0], "cat") == 0)
	{
		if (pipe_mode) return -2;
		parsestr(split_cmd[2]);
		char *path = convert_path(split_cmd[2]);
		cat_file(path);
		free(path);
		return 3;
	}

	else if (strcmp(split_cmd[0], "insert") == 0)
	{
		int _file = 0, _str = 0, _pos = 0;
		int i;
		for (i = 1; ; i++)
		{
			if (pipe_mode && _pos && (split_cmd[i] == NULL || split_cmd[i][0] == '=')) break;
			if (!pipe_mode && _pos && _str && (split_cmd[i] == NULL || split_cmd[i][0] == '=')) break;

			if (strcmp(split_cmd[i], "--file") == 0) _file = ++i;
			else if (strcmp(split_cmd[i], "--str") == 0)
			{
				if (pipe_mode) return -3;
				_str = ++i;
			}
			else if (strcmp(split_cmd[i], "--pos") == 0) _pos = ++i;
		}
		
		if (_file) parsestr(split_cmd[_file]);
		if (!pipe_mode) parsestr(split_cmd[_str]);

		if (pipe_mode)
			insert_command((_file ? split_cmd[_file] : NULL), split_cmd[_pos], pipebuf.arr, pipebuf.len);
		else
			insert_command((_file ? split_cmd[_file] : NULL), split_cmd[_pos], split_cmd[_str], strlen(split_cmd[_str]));
		return i;
	}

	else if (strcmp(split_cmd[0], "remove") == 0 || \
			strcmp(split_cmd[0], "copy") == 0 || \
			strcmp(split_cmd[0], "cut") == 0)
	{
		if (pipe_mode) return -2;
		int _file = 0, _pos = 0, _size = 0, _fw = 0;
		int i;
		for (i = 1; ; i++)
		{
			if (_pos && _size && (split_cmd[i] == NULL || split_cmd[i][0] == '=')) break;

			if (strcmp(split_cmd[i], "--file") == 0) _file = ++i;
			else if (strcmp(split_cmd[i], "--pos") == 0) _pos = ++i;
			else if (strcmp(split_cmd[i], "-size") == 0) _size = ++i;
			else if (strcmp(split_cmd[i], "-f") == 0) _fw = 1;
		}

		if (_file) parsestr(split_cmd[_file]);

		void (*action)(size_t, size_t);
		if (strcmp(split_cmd[0], "remove") == 0) action = removestr;
		else if (strcmp(split_cmd[0], "copy") == 0) action = copystr;
		else action = cutstr;
		selection_action(split_cmd, _file, _pos, _size, _fw, action);
		return i;
	}

	else if (strcmp(split_cmd[0], "paste") == 0)
	{
		if (pipe_mode) return -2;
		int _file = 0, _pos = 0;
		int i;
		for (i = 1; ; i++)
		{
			if (split_cmd[i] == NULL || split_cmd[i][0] == '=') break;

			if (strcmp(split_cmd[i], "--file") == 0) _file = ++i;
			else if (strcmp(split_cmd[i], "--pos") == 0) _pos = ++i;
		}
		if (_file) parsestr(split_cmd[_file]);
		paste_command(split_cmd, _file, _pos);
		return i;
	}

	else if (strcmp(split_cmd[0], "compare") == 0)
	{
		if (pipe_mode) return -2;
		parsestr(split_cmd[1]);
		parsestr(split_cmd[2]);
		char *path1 = convert_path(split_cmd[1]);
		char *path2 = convert_path(split_cmd[2]);
		compare(path1, path2);
		free(path1);
		free(path2);
		return 3;
	}

	else if (strcmp(split_cmd[0], "undo") == 0)
	{
		if (pipe_mode) return -2;
		if (split_cmd[1] == NULL || split_cmd[1][0] == '=')
		{
			undo_buf();
			return 1;
		}
		else
		{
			parsestr(split_cmd[2]);
			char *path = convert_path(split_cmd[2]);
			undo(path);
			free(path);
			return 3;
		}
	}

	else if (strcmp(split_cmd[0], "auto-indent") == 0)
	{
		if (pipe_mode) return -2;
		if (split_cmd[1] == NULL || split_cmd[1][0] == '=')
		{
			create_backup_buf();
			auto_indent_buf(&buf);
			cursor_idx = 0;
			refresh_buffer_vars(&buf);
			is_saved = 0;
			return 1;
		}
		else
		{
			parsestr(split_cmd[1]);
			char *path = convert_path(split_cmd[1]);
			auto_indent_file(path);
			free(path);
			return 2;
		}
	}

	else if (strcmp(split_cmd[0], "tree") == 0)
	{
		if (pipe_mode) return -2;
		long depth = strtoll(split_cmd[1], NULL, 10);
		if (depth >= -1)
		{
			String indent_stack;
			string_init(&indent_stack, 0);
			string_resize(&indent_stack, 64);
			char *cwd = getcwd(NULL, 0);
			tree(ROOT_DIR, &indent_stack, (size_t)depth, 0);
			chdir(cwd);
			free(cwd);
			string_free(&indent_stack);
		}
		else
		{
			print_msg("Error: Invalid depth");
		}
		return 2;
	}

	else if (strcmp(split_cmd[0], "grep") == 0)
	{
		int _file1 = 0, _filen = 0, _str = 0, _c = 0, _l = 0;
		int i;
		for (i = 1; ; i++)
		{
			if (pipe_mode && (split_cmd[i] == NULL || split_cmd[i][0] == '=')) break;
			if (!pipe_mode && _str && (split_cmd[i] == NULL || split_cmd[i][0] == '=')) break;

			if (strcmp(split_cmd[i], "--str") == 0)
			{
				if (pipe_mode) return -3;
				_str = ++i;
			}
			else if (strcmp(split_cmd[i], "-c") == 0) _c = 1;
			else if (strcmp(split_cmd[i], "-l") == 0) _l = 1;
			else if (strcmp(split_cmd[i], "--files") == 0)
			{
				_file1 = ++i;
				while (split_cmd[i + 1] != NULL && split_cmd[i + 1][0] == '/') i++;
				_filen = i;
			}
		}

		if (!pipe_mode) parsestr(split_cmd[_str]);
		for (int i = _file1; i <= _filen; i++) parsestr(split_cmd[i]);

		if (pipe_mode)
		{
			string_null_terminate(&pipebuf);
			grep_command(split_cmd, _file1, _filen, _c, _l, pipebuf.arr);
		}
		else
			grep_command(split_cmd, _file1, _filen, _c, _l, split_cmd[_str]);

		return i;
	}

	else if (strcmp(split_cmd[0], "find") == 0)
	{
		int _file = 0, _str = 0, _count = 0, _has_at = 0, _at = 0, _all = 0, _byword = 0;
		int i;
		for (i = 0; ; i++)
		{
			if (pipe_mode && (split_cmd[i] == NULL || split_cmd[i][0] == '=')) break;
			if (!pipe_mode && _str && (split_cmd[i] == NULL || split_cmd[i][0] == '=')) break;

			if (strcmp(split_cmd[i], "--file") == 0) _file = ++i;
			else if (strcmp(split_cmd[i], "--str") == 0)
			{
				if (pipe_mode) return -3;
				_str = ++i;
			}
			else if (strcmp(split_cmd[i], "-count") == 0) _count = 1;
			else if (strcmp(split_cmd[i], "-all") == 0) _all = 1;
			else if (strcmp(split_cmd[i], "-byword") == 0) _byword = 1;
			else if (strcmp(split_cmd[i], "-at") == 0)
			{
				_at = (int)strtoll(split_cmd[++i], NULL, 10);
				_has_at = 1;
			}
		}
		parsestr(split_cmd[_file]);

		if (pipe_mode)
		{
			string_null_terminate(&pipebuf);
			find_command(split_cmd[_file], _count, _has_at, _at, _all, _byword, pipebuf.arr);
		}
		else
			find_command(split_cmd[_file], _count, _has_at, _at, _all, _byword, split_cmd[_str]);
		return i;
	}

	else if (strcmp(split_cmd[0], "replace") == 0)
	{
		int _file = 0, _old = 0, _new = 0, _has_at = 0, _at = 0, _all = 0;
		int i;
		for (i = 0; ; i++)
		{
			if (pipe_mode && _new && (split_cmd[i] == NULL || split_cmd[i][0] == '=')) break;
			if (!pipe_mode && _old && _new && (split_cmd[i] == NULL || split_cmd[i][0] == '=')) break;

			if (strcmp(split_cmd[i], "--file") == 0) _file = ++i;
			else if (strcmp(split_cmd[i], "--old") == 0)
			{
				if (pipe_mode) return -3;
				_old = ++i;
			}
			else if (strcmp(split_cmd[i], "--new") == 0) _new = ++i;
			else if (strcmp(split_cmd[i], "-all") == 0) _all = 1;
			else if (strcmp(split_cmd[i], "-at") == 0)
			{
				_at = (int)strtoll(split_cmd[++i], NULL, 10);
				_has_at = 1;
			}
		}

		parsestr(split_cmd[_file]);
		parsestr(split_cmd[_new]);

		if (pipe_mode)
		{
			string_null_terminate(&pipebuf);
			replace_command(split_cmd[_file], pipebuf.arr, split_cmd[_new], _has_at, _at, _all);
		}
		else
			replace_command(split_cmd[_file], split_cmd[_old], split_cmd[_new], _has_at, _at, _all);

		return i;
	}

	else if (strcmp(split_cmd[0], "open") == 0)
	{
		parsestr(split_cmd[1]);
		char *path = convert_path(split_cmd[1]);
		open_file(path);
		free(path);
		return 2;
	}

	else if (strcmp(split_cmd[0], "save") == 0)
	{
		save_file();
		return 1;
	}
	
	else if (strcmp(split_cmd[0], "saveas") == 0)
	{
		parsestr(split_cmd[1]);
		char *path = convert_path(split_cmd[1]);
		saveas_file(path);
		free(path);
		return 2;
	}

	return -1;
}

int parse_line(char *cmd)
{
	if (strcmp(cmd, "exit\n") == 0 || strncmp(cmd, "exit ", 5) == 0)
		return 1;

	// split the command line
	char *split_cmd[MAX_TOKENS];
	int tokens = split(cmd, split_cmd);
	if (tokens == 0) return 0;

	int ret = parse_command(split_cmd, 0);
	int idx = ret + 1;
	while (ret >= 0 && split_cmd[idx - 1] != NULL)
	{
		ret = parse_command(split_cmd + idx, 1);
		string_free(&pipebuf);
		idx += ret + 1;
	}

	for (int i = 0; i < tokens; i++)
		free(split_cmd[i]);

	if (ret == -1)
	{
		print_msg("Error: Invalid command");
	}
	else if (ret == -2)
	{
		print_msg("Error: Invalid command after pipe");
	}
	else if (ret == -3)
	{
		print_msg("Error: Unnecessary --str found after pipe");
	}
	else
		ret = 0;

	return ret;
}
