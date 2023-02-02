#include "utils.h"

char *convert_path(char *path)
{
	char *res = malloc(strlen(path) + strlen(ROOT_DIR) - 4);
	strcpy(res, ROOT_DIR);
	strcat(res, path + 5);
	return res;
}

void print_msg(char *msg)
{
	puts(msg);
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
