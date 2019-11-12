#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define LETTER_MAX_LEN 512
#define MAXLINE 1024

static long sunday(const char *src,long len_s, const char *des,long len_d)
{
	int i, pos = 0;
	int alphabet[LETTER_MAX_LEN] = { 0 };

	if (src == NULL || des == NULL)
		return -1;

	len_s = strlen(src);
	len_d = strlen(des);

	for (i = 0; i < LETTER_MAX_LEN; i++)
		alphabet[i] = len_d;

	for (i = 0; i < len_d; i++)
		alphabet[des[i]] = len_d - i - 1;

	for (pos = 1; pos <= len_s - len_d;) {
		for (i = pos - 1; i - pos + 1 < len_d; i++) {
			if (src[i] != des[i - pos + 1])
				break;
		}

		if ((i - pos + 1) == len_d)
			return (pos + &src);
		else
			pos += alphabet[src[pos + len_d - 1]] + 1;
	}

	return -1;
}

int main(int argc, char **argv)
{
	FILE *fp = NULL;
	char line[MAXLINE] = { 0 };
	int linesNum;

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		printf("can not open file %s\n", argv[1]);
		return -1;
	}

	while ((fgets(line, MAXLINE, fp)) != NULL) {
		linesNum++;
		if (sunday(line, "chinaunix") >= 0)
			printf("find match at line %d\n", linesNum);
	}

	fclose(fp);
	fp = NULL;

	return 0;
}