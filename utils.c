#include "utils.h"

char *convert_path(char *path)
{
	char *res = malloc(strlen(path) + strlen(ROOT_DIR) - 4);
	strcpy(res, ROOT_DIR);
	strcat(res, path + 5);
	return res;
}

int address_error(char *path)
{
	char *ptr = strrchr(path, '/');
	*ptr = 0;
	if (access(path, F_OK) != 0)
	{
		print_msg("Error: Address not found");
		*ptr = '/';
		return 1;
	}
	*ptr = '/';
	return 0;
}

int file_not_found_error(char *path)
{
	if (access(path, F_OK) != 0)
	{
		print_msg("Error: File doesn't exist");
		return 1;
	}
	return 0;
}

size_t get_file_size(FILE *fp)
{
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	rewind(fp);
	return size;
}

int count_file_lines(FILE *fp)
{
	long cursor = ftell(fp);
	rewind(fp);
	int ans = 0;
	int ch = fgetc(fp);
	int prev = 0;
	while (ch != EOF)
	{
		if (ch == '\n') ans++;
		prev = ch;
		ch = fgetc(fp);
	}
	if (prev != '\n') ans++;
	fseek(fp, cursor, SEEK_SET);
	return ans;
}

char *get_backup_path(char *path)
{
	char *backup_path = malloc(strlen(path) + 9);
	char *slash = strrchr(path, '/');
	*slash = 0;
	strcpy(backup_path, path);
	strcat(backup_path, "/.");
	strcat(backup_path, slash + 1);
	strcat(backup_path, ".backup");
	*slash = '/';
	return backup_path;
}

void copy_file(char *src, char *dst)
{
	FILE *fr = fopen(src, "r");
	FILE *fw = fopen(dst, "w");
	int ch = fgetc(fr);
	while (ch != EOF)
	{
		fputc(ch, fw);
		ch = fgetc(fr);
	}
	fclose(fr);
	fclose(fw);
}

int count_digits(int num)
{
	int cnt = 0;
	while (num)
	{
		num /= 10;
		cnt++;
	}
	return cnt;
}

size_t pos2idx(String *buf, size_t line_num, size_t idx)
{
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

size_t posstr2idx(String *buf, char *posstr)
{
	char *ptr = strchr(posstr, ':');
	*ptr = 0;
	size_t line_num = strtoull(posstr, NULL, 10);
	size_t idx = strtoull(ptr + 1, NULL, 10);
	*ptr = ':';
	return pos2idx(buf, line_num, idx);
}