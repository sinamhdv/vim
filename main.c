#include "main.h"

String buf;		// current file buffer
String outbuf;	// command output buffer
String clip;	// clipboard
String pipebuf;	// pipes buffer

size_t pos2idx(String *buf, char *posstr)
{
	char *ptr = strchr(posstr, ':');
	*ptr = 0;
	size_t line_num = strtoull(posstr, NULL, 10);
	size_t idx = strtoull(ptr + 1, NULL, 10);
	*ptr = ':';
	size_t i;
	for (i = 0; line_num > 1 && i < buf->len; i++)
	{
		if (buf->arr[i] == '\n') line_num--;
	}
	if (line_num != 1) return -1;
	if (i + idx > buf->len) return -1;
	for (int j = i; j < i + idx; j++)
		if (buf->arr[j] == '\n')
			return -1;
	return i + idx;
}

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
	char *path = convert_path(filename);
	if (load_buffer(&buf, path) != -1)
	{
		size_t idx = pos2idx(&buf, posstr);
		if (idx == -1)
		{
			print_msg("Error: Invalid position");
		}
		else
		{
			string_insert(&buf, str, slen, idx);
			save_buffer(&buf, path);
		}
	}
	free(path);
}

void selection_action(char *split_cmd[], int _file, int _pos, int _size, int _fw, void (*action)(size_t, size_t))
{
	char *path = convert_path(split_cmd[_file]);
	if (load_buffer(&buf, path) != -1)
	{
		size_t idx = pos2idx(&buf, split_cmd[_pos]);
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
				action(L, R);
				save_buffer(&buf, path);
			}
		}
	}
	free(path);
}

void removestr(size_t L, size_t R)
{
	string_remove(&buf, L, R);
}

void copystr(size_t L, size_t R)
{
	string_free(&clip);
	string_init(&clip, R - L);
	memcpy(clip.arr, buf.arr + L, R - L);
}

void cutstr(size_t L, size_t R)
{
	copystr(L, R);
	removestr(L, R);
}

void pastestr(size_t idx)
{
	string_insert(&buf, clip.arr, clip.len, idx);
}

void paste_command(char *split_cmd[], int _file, int _pos)
{
	char *path = convert_path(split_cmd[_file]);
	if (load_buffer(&buf, path) != -1)
	{
		size_t idx = pos2idx(&buf, split_cmd[_pos]);
		if (idx == -1)
		{
			print_msg("Error: Invalid position");
		}
		else
		{
			pastestr(idx);
			save_buffer(&buf, path);
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
	fprintf(stderr, "DBG: undofile(%s) => %s\n", path, backup_path);

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
	if (load_buffer(&buf, path) == -1) return;
	if (auto_indent_buf(&buf) == -1) return;
	save_buffer(&buf, path);
}

int main(void)
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
		if (clip.arr != NULL) fprintf(stderr, "DBG: CLIP[%lu] = '%s'\n", clip.len, clip.arr);
	}

	// free allocated memory
	string_free(&buf);
	string_free(&outbuf);
	string_free(&clip);

	return 0;
}
