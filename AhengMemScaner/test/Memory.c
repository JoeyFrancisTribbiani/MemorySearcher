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

#include "common.h"
#include "scanmem.h"
#include "commands.h"
#include "show_message.h"
//#include "handlers.h"

#include "menu.h"

const char *READ_FAIL = "READFAIL";
const char *WRITE_SUCCESS = "WRITESUCCESS";

/* Returns (scan_data_type_t)(-1) on parse failure */
static inline scan_data_type_t parse_scan_data_type(const char *str)
{
	/* Anytypes */
	if ((strcasecmp(str, "number") == 0) ||
	    (strcasecmp(str, "anynumber") == 0))
		return ANYNUMBER;
	if ((strcasecmp(str, "int") == 0) || (strcasecmp(str, "anyint") == 0) ||
	    (strcasecmp(str, "integer") == 0) ||
	    (strcasecmp(str, "anyinteger") == 0))
		return ANYINTEGER;
	if ((strcasecmp(str, "float") == 0) ||
	    (strcasecmp(str, "anyfloat") == 0))
		return ANYFLOAT;

	/* Ints */
	if ((strcasecmp(str, "i8") == 0) || (strcasecmp(str, "int8") == 0) ||
	    (strcasecmp(str, "integer8") == 0))
		return INTEGER8;
	if ((strcasecmp(str, "i16") == 0) || (strcasecmp(str, "int16") == 0) ||
	    (strcasecmp(str, "integer16") == 0))
		return INTEGER16;
	if ((strcasecmp(str, "i32") == 0) || (strcasecmp(str, "int32") == 0) ||
	    (strcasecmp(str, "integer32") == 0))
		return INTEGER32;
	if ((strcasecmp(str, "i64") == 0) || (strcasecmp(str, "int64") == 0) ||
	    (strcasecmp(str, "integer64") == 0))
		return INTEGER64;

	/* Floats */
	if ((strcasecmp(str, "f32") == 0) || (strcasecmp(str, "float32") == 0))
		return FLOAT32;
	if ((strcasecmp(str, "f64") == 0) ||
	    (strcasecmp(str, "float64") == 0) ||
	    (strcasecmp(str, "double") == 0))
		return FLOAT64;

	/* VLT */
	if (strcasecmp(str, "bytearray") == 0)
		return BYTEARRAY;
	if (strcasecmp(str, "string") == 0)
		return STRING;

	/* Not a valid type */
	return (scan_data_type_t)(-1);
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

const char *set_scan_data_type(globals_t *vars, const char *type_str)
{
	const char *type;
	if (!strcmp(type_str, "I8"))
		type = "int8";
	if (!strcmp(type_str, "U8"))
		type = "int8";
	if (!strcmp(type_str, "I16"))
		type = "int16";
	if (!strcmp(type_str, "U16"))
		type = "int16";
	if (!strcmp(type_str, "I32"))
		type = "int32";
	if (!strcmp(type_str, "U32"))
		type = "int32";
	if (!strcmp(type_str, "I64"))
		type = "int64";
	if (!strcmp(type_str, "U64"))
		type = "int64";
	if (!strcmp(type_str, "F32"))
		type = "float32";
	if (!strcmp(type_str, "F64"))
		type = "float64";
	if (!strcmp(type_str, "String"))
		type = "string";

	scan_data_type_t st = parse_scan_data_type(type);
	//printf("datatype:%d", st);
	if (st != (scan_data_type_t)(-1)) {
		vars->options.scan_data_type = st;
	} else {
		printf("bad value for scan_data_type, see `help option`.\n");
	}
	return type;
}

bool get_addrs(globals_t *vars, lua_State *L,list_t* matched)
{
	//char *cmd = "list ";
	//sm_execcommand(vars, cmd);
	//lua_newtable(L); //创建一个表格，放在栈顶
	printf("in get_addrs and list done and new table.\n");

	//int index = 1;
	////element_t *np = matched->head;
	//element_t *np = matched->head;
	////lua_settop(L,0);
	//lua_newtable(L); //创建一个表格，放在栈顶
	//printf("in get_addrs and list done and new table.\n");
	//while (np) {
	//	unsigned long addr = np->data;
	//	printf("this is c print:%llx", addr);
	//	lua_pushinteger(L, addr);
	//	lua_rawseti(L, -2, index); //set array[0] by -1
	//	np = np->next;
	//	index++;
	//}
	//// 清理栈顶
	//lua_pop(L, 1);
}

bool match_arr_addrs(globals_t *vars, lua_State *L,long long len)
{
	char *cmd = "list ";
	//char cmd[strlen(cmd_head) + strlen(data) + 1];
	//sprintf(cmd, "%s%s", cmd_head, data);
	//printf(cmd);
	sm_execcommand(vars, cmd);

	list_t *matched = l_init();

	// 对第一次搜索得到的所有地址挨个进行附加条件的校验
	element_t *np = vars->addrs->head;
	while (np) {
		bool all_matched = true;
		// 从第一个条件开始
		element_t *condition = vars->data_arr->head->next;
		while (condition) {
			// 第一个条件项存储地址偏移
			long long check_addr = (long long)np->data + (long long)condition->data;
			long long check_mem_value =
				read_memory(vars->target, check_addr, len);

			// 把读到的值和第二个条件项的值比较
			long long accept_value = condition->next->data;
			// 相等继续往下比较
			if (check_mem_value == accept_value) {
				condition = condition->next->next;
			}
			// 不相等就break
			else {
				all_matched = false;
				break;
			}
		}
		if (all_matched) {
			l_append(matched, matched->tail, np->data);
		}
		np = np->next;
	}
	get_addrs(vars, L, matched);
}
// Read(pkg_name,addr,type)
static int Read(lua_State *L)
{
	const char *pkg_name = luaL_checkstring(L, 1);
	long long start_addr = luaL_checkinteger(L, 2);
	const char *type_str = luaL_checkstring(L, 3);

	int pid = find_pid_of(pkg_name);
	int len = type_to_len(type_str);

	unsigned char *data = read_memory(pid, start_addr, len);

	if (!strcmp((const char *)data, READ_FAIL)) {
		printf("Read fail");
		lua_pushboolean(L, 0);
		lua_pushstring(L, "read failed");
	} else {
		printf("Read success");
		lua_pushboolean(L, 1);
		lua_pushstring(L, data);
	}
	return 2;
}

// Write(pkg_name,addr,data,type)
static int Write(lua_State *L)
{
	const char *pkg_name = luaL_checkstring(L, 1);
	long long start_addr = luaL_checkinteger(L, 2);
	const char *data = luaL_checkstring(L, 3);
	const char *type_str = luaL_checkstring(L, 4);

	if (scanmem_init(pkg_name, type_str) == EXIT_FAILURE) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "permission error");
		return 2;
	}

	globals_t *vars = &sm_globals;

	char *cmd_head = "write";
	const char *cmd_data_type = set_scan_data_type(vars, type_str);
	char cmd[strlen(cmd_head) + sizeof(long long) + strlen(cmd_data_type) +
		 strlen(data) + 1];
	sprintf(cmd, "%s %s %llx %s", cmd_head, cmd_data_type, start_addr,
		data);
	bool success = sm_execcommand(vars, cmd);

	if (!success) {
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
	const char *pkg_name = luaL_checkstring(L, 1);
		const char *data = luaL_checkstring(L, 2);
	const char *type_str = luaL_checkstring(L, 3);

	//lua_newtable(L); //创建一个表格，放在栈顶
	printf("in get_addrs and list done and new table.\n");

	globals_t *vars = &sm_globals;
	char *cmd_head = "\" ";

	char cmd[strlen(cmd_head) + strlen(data) + 1];
	sprintf(cmd, "%s%s", cmd_head, data);
	sm_execcommand(vars, cmd);

	get_addrs(vars, L, vars->addrs);
	return 1;

	//globals_t *vars = &sm_globals;
	//if (strcmp(type_str, "String") == 0) {

	//	const char *data = luaL_checkstring(L, 2);

	//	if (scanmem_init(pkg_name, type_str) == EXIT_FAILURE) {
	//		lua_pushboolean(L, 0);
	//		lua_pushstring(L, "permission error");
	//		return 2;
	//	}
	//	char *cmd_head = "\" ";

	//	char cmd[strlen(cmd_head) + strlen(data) + 1];
	//	sprintf(cmd, "%s%s", cmd_head, data);
	//	sm_execcommand(vars, cmd);

	//	get_addrs(vars, L, vars->addrs);

	//	return 1;
	//} else {
	//	int arr_count = 0;
	//	lua_pushvalue(L, 2);
	//	lua_pushnil(L);
	//	while (lua_next(L, -2)) {
	//		long long key = lua_tonumber(L, -2);
	//		long long value = lua_tointeger(L, -1);
	//		l_append(vars->data_arr, vars->data_arr->tail, value);
	//		lua_pop(L, 1);
	//		arr_count++;
	//	}
	//	lua_pop(L, 1);
	//	//printf("pkg_name:%s,data:%s,type:%s", pkg_name, data, type_str);

	//	if (scanmem_init(pkg_name, type_str) == EXIT_FAILURE) {
	//		lua_pushboolean(L, 0);
	//		lua_pushstring(L, "permission error");
	//		return 2;
	//	}

	//	char *cmd_head = "\" ";
	//	/*if (strcmp(type_str, "String") != 0) {
	//		cmd_head = "= ";
	//	}*/
	//	char cmd[strlen(cmd_head) + sizeof(long) + 1];
	//	sprintf(cmd, "%s%s", cmd_head, vars->data_arr->head->data);
	//	sm_execcommand(vars, cmd);

	//	long long len = type_to_len(type_str);
	//	match_arr_addrs(vars, L, len);
	//	return 1;
	//}
}
static int hello(lua_State *L)
{
	printf("fuck me c++");
	return 0;
}
int luaopen_Memory(lua_State *L)
{
	luaL_Reg l[] = { { "Read", Read },
			 { "Write", Write },
			 { "Search", Search },
			 { "Hello", hello },
			 { NULL, NULL } };
	luaL_newlib(L, l);
	return 1;
}

int scanmem_init(const char *pkg_name, const char *type_str)
{
	int ret = EXIT_SUCCESS;
	globals_t *vars = &sm_globals;

	if (!sm_init()) {
		show_error("Initialization failed.\n");
		sm_cleanup();
		return EXIT_FAILURE;
	}

	if (getuid() != 0) {
		show_warn("Run scanmem as root if memory regions are missing. "
			  "See scanmem man page.\n\n");
	}

	/* this will initialize matches and regions */
	if (sm_execcommand(vars, "reset") == false) {
		vars->target = 0;
	}

	// 设置搜索的类型
	set_scan_data_type(vars, type_str);
	printf("vars->scan_type:%d\n", vars->options.scan_data_type);

	// 获取pid，调用pid cmd
	pid_t pid = find_pid_of(pkg_name);
	char *cmd_head = "pid ";
	//char *cmd = new char[strlen(cmd_head) + sizeof(pid) + 1];
	char cmd[strlen(cmd_head) + sizeof(pid) + 1];
	// 设置搜索的进程pid
	sprintf(cmd, "%s %d", cmd_head, pid);
	//printf("getpid:%d", pid);
	sm_execcommand(vars, cmd);
	return EXIT_SUCCESS;
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
		printf("getpid:process name is null");
		return -1;
	}

	dir = opendir("/proc");
	if (dir == NULL) {
		printf("getpid:/proc is null");
		return -1;
	}

	while ((entry = readdir(dir)) != NULL) {
		id = atoi(entry->d_name);
		if (id != 0) {
			sprintf(filename, "/proc/%d/cmdline", id);
			fp = fopen(filename, "r");
			if (fp) {
				fgets(cmdline, sizeof(cmdline), fp);
				if (id == 42837) {
					printf("proc/cmdline:%s\n", cmdline);
				}
				fclose(fp);
			}

			char pc_cmd[strlen(process_name) + 2];
			sprintf(pc_cmd, "./%s", process_name);
			if (strcmp(process_name, cmdline) == 0 ||
			    strcmp(pc_cmd, cmdline) == 0) {
				pid = id;
				break;
			}
		}
	}
	closedir(dir);
	return pid;
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
		return 128;
}
