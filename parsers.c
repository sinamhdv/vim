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

		char *ptr = res[size];
		for (int i = 0; i < end - cmd - 1; i++)
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
		cmd = end;
		size++;
	}
	res[size] = NULL;
	return size;
}

int parse_command(char *split_cmd[])
{
	if (strcmp(split_cmd[0], "create") == 0)
	{
		char *path = convert_path(split_cmd[2]);
		create_file(path);
		free(path);
		return 3;
	}
	else if (strcmp(split_cmd[0], "cat") == 0)
	{
		char *path = convert_path(split_cmd[2]);
		cat_file(path);
		free(path);
		return 3;
	}
	else if (strcmp(split_cmd[0], "insert") == 0)
	{
		int _file = 0, _str = 0, _pos = 0;
		for (int i = 1; split_cmd[i] != NULL; i++)
		{
			if (strcmp(split_cmd[i], "--file") == 0) _file = ++i;
			else if (strcmp(split_cmd[i], "--str") == 0) _str = ++i;
			else if (strcmp(split_cmd[i], "--pos") == 0) _pos = ++i;
		}
		insert_command(split_cmd, _file, _str, _pos);
		return 7;
	}
	else if (strcmp(split_cmd[0], "remove") == 0 || \
			strcmp(split_cmd[0], "copy") == 0 || \
			strcmp(split_cmd[0], "cut") == 0)
	{
		int _file = 0, _pos = 0, _size = 0, _fw = 0;
		for (int i = 1; split_cmd[i] != NULL; i++)
		{
			if (strcmp(split_cmd[i], "--file") == 0) _file = ++i;
			else if (strcmp(split_cmd[i], "--pos") == 0) _pos = ++i;
			else if (strcmp(split_cmd[i], "-size") == 0) _size = ++i;
			else if (strcmp(split_cmd[i], "-f") == 0) _fw = 1;
		}
		void (*action)(size_t, size_t);
		if (strcmp(split_cmd[0], "remove") == 0) action = removestr;
		else if (strcmp(split_cmd[0], "copy") == 0) action = copystr;
		else action = cutstr;
		selection_action(split_cmd, _file, _pos, _size, _fw, action);
		return 8;
	}
	else if (strcmp(split_cmd[0], "paste") == 0)
	{
		int _file = 0, _pos = 0;
		for (int i = 1; split_cmd[i] != NULL; i++)
		{
			if (strcmp(split_cmd[i], "--file") == 0) _file = ++i;
			else if (strcmp(split_cmd[i], "--pos") == 0) _pos = ++i;
		}
		paste_command(split_cmd, _file, _pos);
		return 5;
	}
	else if (strcmp(split_cmd[0], "compare") == 0)
	{
		char *path1 = convert_path(split_cmd[1]);
		char *path2 = convert_path(split_cmd[2]);
		compare(path1, path2);
		free(path1);
		free(path2);
		return 3;
	}
	else if (strcmp(split_cmd[0], "undo") == 0)
	{
		char *path = convert_path(split_cmd[2]);
		undo(path);
		free(path);
		return 3;
	}
	else if (strcmp(split_cmd[0], "auto-indent") == 0)
	{
		char *path = convert_path(split_cmd[1]);
		auto_indent_file(path);
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
	
	int idx = parse_command(split_cmd);
	while (split_cmd[idx] != NULL)
	{
		idx = parse_command(split_cmd + idx + 1);
		if (idx == -1)
		{
			print_msg("Error: Invalid command");
			break;
		}
	}

	// clean up
	for (int i = 0; i < tokens; i++)
		free(split_cmd[i]);
	return 0;
}
