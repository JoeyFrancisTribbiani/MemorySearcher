#include "lua.h"
#include "lauxlib.h"
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>
#include "Sunday.c"

const char *READ_FAIL = "READFAIL";
const char *WRITE_SUCCESS = "WRITESUCCESS";

// Read(pkg_name,addr,type)
static int Read(lua_State *L)
{
	char *pkg_name = luaL_checkstring(L, 1);
	long long start_addr = luaL_checkinteger(L, 2);
	char *type_str = luaL_checkstring(L, 3);

	int pid = find_pid_of(pkg_name);
	int len = type_to_len(type_str);

	unsigned char *data = read_memory(pid, start_addr, len);

	if (!strcmp(data, READ_FAIL)) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "read failed");
	} else {
		lua_pushboolean(L, 1);
		lua_pushstring(L, data);
	}
	return 2;

	//if (!strcmp(type_str, "I8")
	//	|| !strcmp(type_str, "U8")
	//	|| !strcmp(type_str, "I16")
	//	|| !strcmp(type_str, "U16")
	//	|| !strcmp(type_str, "I32")
	//	|| !strcmp(type_str, "U32")
	//	|| !strcmp(type_str, "I64")
	//	|| !strcmp(type_str,"U64")
	//	|| !strcmp(type_str,"F32")
	//	|| !strcmp(type_str,"F64")) {

	//}
	//if (!strcmp(type_str, "String"))
	//	return 64;
}

// Write(pkg_name,addr,data,type)
static int Write(lua_State *L)
{
	char *pkg_name = luaL_checkstring(L, 1);
	long long start_addr = luaL_checkinteger(L, 2);
	char *data = luaL_checkstring(L, 3);
	char *type_str = luaL_checkstring(L, 4);

	int pid = find_pid_of(pkg_name);
	int len = type_to_len(type_str);

	unsigned char *data = write_memory(pid, start_addr, data, len);

	if (!strcmp(data, READ_FAIL)) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "permission error");
	} else {
		lua_pushboolean(L, 1);
		lua_pushstring(L, "write success");
	}
	return 2;
}

static int Search(lua_State *L)
{
	char *pkg_name = luaL_checkstring(L, 1);
	char *data = luaL_checkstring(L, 2);
	char *type_str = luaL_checkstring(L, 3);

	int pid = find_pid_of(pkg_name);
	int data_len = search_type_to_len(type_str, data);

	long start_addr = 0x40000000‬;
	long end_addr = 0x80000000;
	long src_len = end_addr - start_addr;

	char *src;
	unsigned char *src = read_memory(pid, start_addr, src_len);

	char *result = sunday(src, src_len, data, data_len);
	if (!strcmp(result, -1)) {

		lua_pushboolean(L, 0);
		lua_pushstring(L, "permission error");
	} else {
	}
	return 2;
}

int luaopen_Memory(lua_State *L)
{
	luaL_Reg l[] = { { "Read", Read },
			 { "Write", Write },
			 { "Search", Search },
			 { NULL, NULL } };
	luaL_newlib(L, l);
	return 1;
}

int find_pid_of(const char *process_name)
{
	int id;
	pid_t pid = -1;
	DIR *dir;
	FILE *fp;
	char filename[64];
	char cmdline[296];

	struct dirent *entry;

	if (process_name == NULL) {
		return -1;
	}

	dir = opendir("/proc");
	if (dir == NULL) {
		return -1;
	}

	while ((entry = readdir(dir)) != NULL) {
		id = atoi(entry->d_name);
		if (id != 0) {
			sprintf(filename, "/proc/%d/cmdline", id);
			fp = fopen(filename, "r");
			if (fp) {
				fgets(cmdline, sizeof(cmdline), fp);
				fclose(fp);
			}

			if (strcmp(process_name, cmdline) == 0) {
				pid = id;
				break;
			}
		}
	}
	closedir(dir);
	return pid;
}

unsigned char *read_memory(int pid, long long start_addr, long long len)
{
	/*attach the memory of pid*/
	int ptrace_ret;
	ptrace_ret = ptrace(PTRACE_ATTACH, pid, NULL, NULL);
	if (ptrace_ret == -1) {
		fprintf(stderr, "ptrace attach failed.\n");
		perror("ptrace");
		return READ_FAIL;
	}
	if (waitpid(pid, NULL, 0) == -1) {
		fprintf(stderr, "waitpid failed.\n");
		perror("waitpid");
		ptrace(PTRACE_DETACH, pid, NULL, NULL);
		return READ_FAIL;
	}

	/*open /proc/<pid>/mem to attach the memory*/
	int fd;
	char path[256] = { 0 };
	sprintf(path, "/proc/%d/mem", pid);
	fd = open(path, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "open file failed.\n");
		perror("open");
		ptrace(PTRACE_DETACH, pid, NULL, NULL);
		return READ_FAIL;
	}

	/*seek the file pointer*/
	off_t off;
	off = lseek(fd, start_addr, SEEK_SET);
	if (off == (off_t)-1) {
		fprintf(stderr, "lseek failed.\n");
		perror("lseek");
		ptrace(PTRACE_DETACH, pid, NULL, NULL);
		close(fd);
		return READ_FAIL;
	}

	/*read mem*/
	unsigned char *buf = (unsigned char *)malloc(len);
	if (buf == NULL) {
		fprintf(stderr, "malloc failed.\n");
		perror("malloc");
		ptrace(PTRACE_DETACH, pid, NULL, NULL);
		close(fd);
		return READ_FAIL;
	}
	int rd_sz;
	rd_sz = read(fd, buf, len);
	if (rd_sz < len) {
		fprintf(stderr, "read failed.\n");
		perror("read");
		ptrace(PTRACE_DETACH, pid, NULL, NULL);
		free(buf);
		close(fd);
		return READ_FAIL;
	}

	/*now show mem*/
	/*int i = 0;
	FILE *fp = fopen(argv[4], "wb+");
	if (fp == NULL) {
		fprintf(stderr, "fopen failed.\n");
		perror("fopen");
		ptrace(PTRACE_DETACH, pid, NULL, NULL);
		free(buf);
		close(fd);
		return -1;
	}
	fwrite(buf, 1, len, fp);
	fclose(fp);*/

	ptrace(PTRACE_DETACH, pid, NULL, NULL);
	//free(buf);
	close(fd);
	return buf;
}

unsigned char *write_memory(int pid, char *dest, char *data, size_t size)
{
	/*attach the memory of pid*/
	int ptrace_ret;
	ptrace_ret = ptrace(PTRACE_ATTACH, pid, NULL, NULL);
	if (ptrace_ret == -1) {
		fprintf(stderr, "ptrace attach failed.\n");
		perror("ptrace");
		return READ_FAIL;
	}
	if (waitpid(pid, NULL, 0) == -1) {
		fprintf(stderr, "waitpid failed.\n");
		perror("waitpid");
		ptrace(PTRACE_DETACH, pid, NULL, NULL);
		return READ_FAIL;
	}

	long i, j, remain;
	uint8_t *laddr;

	union u {
		long val;
		char u_data[sizeof(long)];
	} d;

	j = size / 4;
	remain = size % 4;

	laddr = data;

	//先4字节拷贝
	for (i = 0; i < j; i++) {
		memcpy(d.u_data, laddr, 4);
		//往内存地址中写入四个字节,内存地址由dest给出
		ptrace(PTRACE_POKETEXT, pid, dest, d.val);

		dest += 4;
		laddr += 4;
	}

	//最后不足4字节的，单字节拷贝
	//为了最大程度的保持原栈的数据，需要先把原程序最后四字节读出来
	//然后把多余的数据remain覆盖掉四字节中前面的数据
	if (remain > 0) {
		d.val = ptrace(
			PTRACE_PEEKTEXT, pid, dest,
			0); //从内存地址中读取四个字节，内存地址由dest给出
		for (i = 0; i < remain; i++) {
			d.u_data[i] = *laddr++;
		}
		ptrace(PTRACE_POKETEXT, pid, dest, d.val);
	}

	ptrace(PTRACE_DETACH, pid, NULL, NULL);
	return WRITE_SUCCESS;
}

int type_to_len(char *type_str)
{
	//char type[32];
	//int len = strlcpy(type, type_str, sizeof(type));
	//if (len >= sizeof(type)) {
	//	printf("src is truncated");
	//	return -1;
	//}

	if (!strcmp(type_str, "I8"))
		return 1;
	if (!strcmp(type_str, "U8"))
		return 1;
	if (!strcmp(type_str, "I16"))
		return 2;
	if (!strcmp(type_str, "U16"))
		return 2;
	if (!strcmp(type_str, "I32"))
		return 4;
	if (!strcmp(type_str, "U32"))
		return 4;
	if (!strcmp(type_str, "I64"))
		return 8;
	if (!strcmp(type_str, "U64"))
		return 8;
	if (!strcmp(type_str, "F32"))
		return 4;
	if (!strcmp(type_str, "F64"))
		return 8;
	if (!strcmp(type_str, "String"))
		return 64;
}

int search_type_to_len(char *type_str, char *data)
{
	//char type[32];
	//int len = strlcpy(type, type_str, sizeof(type));
	//if (len >= sizeof(type)) {
	//	printf("src is truncated");
	//	return -1;
	//}

	if (!strcmp(type_str, "I8"))
		return 1;
	if (!strcmp(type_str, "U8"))
		return 1;
	if (!strcmp(type_str, "I16"))
		return 2;
	if (!strcmp(type_str, "U16"))
		return 2;
	if (!strcmp(type_str, "I32"))
		return 4;
	if (!strcmp(type_str, "U32"))
		return 4;
	if (!strcmp(type_str, "I64"))
		return 8;
	if (!strcmp(type_str, "U64"))
		return 8;
	if (!strcmp(type_str, "F32"))
		return 4;
	if (!strcmp(type_str, "F64"))
		return 8;
	if (!strcmp(type_str, "String"))
		return sizeof(data);
}
