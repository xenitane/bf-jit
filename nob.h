/**
 * @file nob.h
 * @author Xenitane<tushar01.tjdsk@gmail.com>
 * @brief This is a cool utility package that I'm making inspired from tsoding's
 * no-build for my own use as well.
 * @brief This file lies in it's own repository
 * <https://www.github.com/xenitane/nob.h>
 * @copyright Copyright (c) 2024
 *
 */

#ifndef _NOB_H_
#define _NOB_H_

#define NOB_ASSERT	assert
#define NOB_REALLOC realloc
#define NOB_FREE	free
#define str_err_no	strerror(errno)

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINUSER_
#define _WINGDI_
#define _IMM_
#define _WINCON_
#include <direct.h>
#include <shellapi.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define NOB_LINE_END "\r\n"
#else
#define NOB_LINE_END "\n"
#endif

#define NOB_ARRAY_LEN(array)		(sizeof(array) / sizeof(*array))
#define NOB_ARRAY_GET(array, index) (NOB_ASSERT(index >= 0), NOB_ASSERT(index < NOB_ARRAY_LEN(array)), array[index])

typedef enum {
	NOB_INFO,
	NOB_WARNING,
	NOB_ERROR,
} NOB_Log_Level;

void nob_log(NOB_Log_Level log_level, const char *fmt, ...);

// equivalent of shift from bash. pops out a cli arg from beginning
char *nob_shift_args(int *argc, char ***argv);

typedef struct {
	const char **items;
	size_t		 count;
	size_t		 capacity;
} NOB_File_Paths;

typedef enum {
	NOB_FILE_REGULAR,
	NOB_FILE_DIRECTORY,
	NOB_FILE_SYMLINK,
	NOB_FILE_OTHER,
} NOB_File_Type;

bool		  nob_mkdir_if_not_exists(const char *path);
bool		  nob_copy_file(const char *src_path, const char *dst_path);
bool		  nob_copy_directory_recursively(const char *src_path, const char *dst_path);
bool		  nob_read_entire_dir(const char *parent, NOB_File_Paths *children);
bool		  nob_write_entire_file(const char *path, const void *data, size_t size);
NOB_File_Type nob_get_file_type(const char *path);

#define nob_return_defer(value)                                                                                                \
	do {                                                                                                                       \
		result = (value);                                                                                                      \
		goto defer;                                                                                                            \
	} while (0)

// initial capacity for a dynamic array
#define NOB_DA_INIT_CAP 256

#define nob_da_append(da, item)                                                                                                \
	do {                                                                                                                       \
		if ((da)->count >= (da)->capacity) {                                                                                   \
			if ((da)->capacity == 0) {                                                                                         \
				(da)->capacity = NOB_DA_INIT_CAP;                                                                              \
			} else {                                                                                                           \
				(da)->capacity *= 2;                                                                                           \
			}                                                                                                                  \
			(da)->items = NOB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));                                     \
			NOB_ASSERT((da)->items != NULL && "Buy more RAM lol");                                                             \
		}                                                                                                                      \
                                                                                                                               \
		(da)->items[(da)->count++] = (item);                                                                                   \
	} while (0)

#define nob_da_free(da) NOB_FREE((da).items)

#define nob_da_append_many(da, new_items, new_items_count)                                                                     \
	do {                                                                                                                       \
		if ((da)->count + new_items_count > (da)->capacity) {                                                                  \
			if ((da)->capacity == 0) { (da)->capacity = NOB_DA_INIT_CAP; }                                                     \
			while ((da)->count + new_items_count > (da)->capacity) { (da)->capacity *= 2; }                                    \
			(da)->items = NOB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));                                     \
			NOB_ASSERT((da)->items != NULL && "Buy more RAM lol");                                                             \
		}                                                                                                                      \
		memcpy((da)->items + (da)->count, new_items, new_items_count * sizeof(*(da)->items));                                  \
		(da)->count += new_items_count;                                                                                        \
	} while (0)

typedef struct {
	char	 *items;
	size_t count;
	size_t capacity;
} NOB_String_Builder;

bool nob_read_entire_file(const char *path, NOB_String_Builder *sb);

// appends sized buffer to string builder
#define nob_sb_append_buf(sb, buf, size) nob_da_append_many(sb, buf, size)

// appends a null terminated C string to string builder
#define nob_sb_append_cstr(sb, cstr)                                                                                           \
	do {                                                                                                                       \
		const char *s = cstr;                                                                                                  \
		size_t		n = strlen(s);                                                                                             \
		nob_da_append_many(sb, s, n);                                                                                          \
	} while (0)

// appends a single NULL character to the end of a string builder. Then you can
// use it as a null terminates string
#define nob_sb_append_null(sb) nob_da_append_many(sb, "", 1)

// frees the memory of the string builder
#define nob_sb_free(sb) NOB_FREE((sb).items)

// process handle
#ifdef _WIN32
typedef HANDLE NOB_Proc;
#define NOB_INVALID_PROC INVALID_HANDLE_VALUE
#else
typedef int NOB_Proc;
#define NOB_INVALID_PROC (-1)
#endif

typedef struct {
	NOB_Proc *items;
	size_t	  count;
	size_t	  capacity;
} NOB_Procs;

bool nob_procs_wait(NOB_Procs procs);

// wait until the process has finished
bool nob_proc_wait(NOB_Proc proc);

typedef struct {
	const char **items;
	size_t		 count;
	size_t		 capacity;
} NOB_Cmd;

// render a string representation of a command into a string builder. Keep in
// mind that the string builder is not null terminated by default. Use
// nob_sb_append_null if you plan to use it as C string
void nob_cmd_render(NOB_Cmd cmd, NOB_String_Builder *render);

#define nob_cmd_append(cmd, ...)                                                                                               \
	nob_da_append_many(cmd, ((const char *[]){__VA_ARGS__}), (sizeof((const char *[]){__VA_ARGS__}) / sizeof(const char *)))

// frees all the memory allocated by the command args
#define nob_cmd_free(cmd) NOB_FREE(cmd.items)

NOB_Proc nob_cmd_run_async(NOB_Cmd cmd);

bool nob_cmd_run_sync(NOB_Cmd cmd);

#ifndef NOB_TEMP_CAPACITY
#define NOB_TEMP_CAPACITY (8 * 1024 * 1024)
#endif

char	 *nob_temp_strdup(const char *cstr);
void	 *nob_temp_alloc(size_t size);
char	 *nob_temp_sprintf(const char *fmt, ...);
void   nob_temp_reset(void);
size_t nob_temp_save(void);
void   nob_temp_rewind(size_t checkpoint);

int	 is_path1_modified_after_path2(const char *path1, const char *path2);
bool nob_rename(const char *old_path, const char *new_path);
int	 nob_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count);
int	 nob_needs_rebuild1(const char *output_path, const char *input_path);
int	 nob_file_exists(const char *file_path);

#ifndef NOB_REBUILD_YOURSELF
#ifdef _WIN32
#if defined(__GNUC__)
#define NOB_REBUILD_YOURSELF(binary_path, source_path) "gcc", "-o", binary_path, source_path
#elif defined(__clang__)
#define NOB_REBUILD_YOURSELF(binary_path, source_path) "clang", "-o", binary_path, source_path
#elif defined(_MSC_VER)
#define NOB_REBUILD_YOURSELF(binary_path, source_path) "cl.exe", source_path
#endif
#else
#define NOB_REBUILD_YOURSELF(binary_path, source_path) "cc", "-o", binary_path, source_path
#endif
#endif

/**
 * Go Rebuild Yourself™ Technology
 *
 *  How to use it:
 * 	int main(int argc, char** argv){
 * 		GO_REBUILD_YOURSELF(argc,argv);
 * 		// code for the program below
 * 		return 0;
 * 	}
 *
 *	After your added this macro every time you run ./nob it will detect
 *	that you modified its original source code and will try to rebuild
 *itself before doing any actual work. So you only need to bootstrap your build
 *system once.
 *
 * 	The modification is detected by comparing the last modified times of the
 *executable and its source code. The same way the make utility usually does it.
 *
 * 	The rebuilding is done by using the REBUILD_YOURSELF macro which you can
 *redefine if you need a special way of bootstrapping your build system. (which
 *I personally do not recommend since the whole idea of no-build is to keep the
 *process of bootstrapping as simple as possible and doing all of the actual
 *work inside of the no-build)
 */
#define GO_REBUILD_YOURSELF(argc, argv)                                                                                        \
	do {                                                                                                                       \
		const char *source_path = __FILE__;                                                                                    \
		assert(argc >= 0);                                                                                                     \
		const char *binary_path		  = argv[0];                                                                               \
		int			rebuild_is_needed = nob_needs_rebuild(binary_path, &source_path, 1);                                       \
		if (rebuild_is_needed < 0) { exit(1); }                                                                                \
		if (rebuild_is_needed) {                                                                                               \
			NOB_String_Builder sb = {0};                                                                                       \
			nob_sb_append_cstr(&sb, binary_path);                                                                              \
			nob_sb_append_cstr(&sb, ".old");                                                                                   \
			nob_sb_append_null(&sb);                                                                                           \
			if (!nob_rename(binary_path, sb.items)) { exit(1); }                                                               \
			NOB_Cmd rebuild = {0};                                                                                             \
			nob_cmd_append(&rebuild, NOB_REBUILD_YOURSELF(binary_path, source_path));                                          \
			bool rebuild_succeeded = nob_cmd_run_sync(rebuild);                                                                \
			nob_cmd_free(rebuild);                                                                                             \
			if (!rebuild_succeeded) {                                                                                          \
				nob_rename(sb.items, binary_path);                                                                             \
				exit(1);                                                                                                       \
			}                                                                                                                  \
			NOB_Cmd cmd = {0};                                                                                                 \
			nob_da_append_many(&cmd, argv, argc);                                                                              \
			if (!nob_cmd_run_sync(cmd)) { exit(1); }                                                                           \
			exit(0);                                                                                                           \
		}                                                                                                                      \
	} while (0)

typedef struct {
	size_t		count;
	const char *data;
} NOB_String_View;

const char *nob_temp_sv_to_cstr(NOB_String_View sv);

NOB_String_View nob_sv_chop_by_delim(NOB_String_View *sv, char delim);
NOB_String_View nob_sv_trim(NOB_String_View sv);
bool			nob_sv_eq(NOB_String_View a, NOB_String_View b);
NOB_String_View nob_sv_from_cstr(const char *cstr);
NOB_String_View nob_sv_from_parts(const char *data, size_t count);

// printf macros for string view
#ifndef SV_Fmt
#define SV_Fmt "%.*s"
#endif
#ifndef SV_Arg
#define SV_Arg(sv) (int)(sv).count, (sv).data
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

struct dirent {
	char d_name[MAX_PATH + 1];
};

typedef struct DIR DIR;

DIR		   *opendir(const char *dirpath);
struct dirent *readdir(DIR *dirp);
int			   closedir(DIR *dirp);
#else
#include <dirent.h>
#endif

#endif

#ifndef _NOB_IMPL_
#define _NOB_IMPL_

#ifdef NOB_IMPL

static size_t nob_temp_size				  = 0;
static char	  nob_temp[NOB_TEMP_CAPACITY] = {0};

bool nob_mkdir_if_not_exists(const char *path) {
#ifdef _WIN32
	int result = mkdir(path);
#else
	int	   result	= mkdir(path, 0755);
#endif
	if (result < 0) {
		if (errno == EEXIST) {
			nob_log(NOB_INFO, "directory `%s` already exists", path);
			return true;
		}
		nob_log(NOB_ERROR, "could not create directory `%s` : %s", path, str_err_no);
		return false;
	}
	nob_log(NOB_INFO, "directory created successfully `%s`", path);
	return true;
}

bool nob_copy_file(const char *src_path, const char *dst_path) {
	nob_log(NOB_INFO, "copying %s -> %s", src_path, dst_path);
#ifdef _WIN32
	if (!CopyFile(src_path, dst_path, false)) {
		nob_log(NOB_ERROR, "Could Not Copy File: %lu", GetLastError());
		return false;
	}
	return true;
#else
	int	   src_fd	= -1;
	int	   dst_fd	= -1;
	size_t buf_size = 32 * 1024;
	char	 *buff		= NOB_REALLOC(NULL, buf_size);
	NOB_ASSERT(buff != NULL && "Buy More RAM lol");
	bool result = false;

	src_fd = open(src_path, O_RDONLY);
	if (src_fd < 0) {
		nob_log(NOB_ERROR, "Could not open file %s: %s", src_path, str_err_no);
		nob_return_defer(false);
	}
	struct stat src_stat;
	if (fstat(src_fd, &src_stat) < 0) {
		nob_log(NOB_ERROR, "Could not get mode of file %s: %s", src_path, str_err_no);
		nob_return_defer(false);
	}

	dst_fd = open(dst_path, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
	if (dst_fd < 0) {
		nob_log(NOB_ERROR, "Could not create file %s: %s", dst_path, str_err_no);
		nob_return_defer(false);
	}

	while (1) {
		ssize_t n = read(src_fd, buff, buf_size);
		if (n == 0) { break; }
		if (n < 0) {
			nob_log(NOB_ERROR, "could not read from file %s: %s", src_path, str_err_no);
			nob_return_defer(false);
		}
		char *buff2 = buff;
		while (n > 0) {
			ssize_t m = write(dst_fd, buff2, n);
			if (m < 0) {
				nob_log(NOB_ERROR, "Could not write to file %s: %s", dst_path, str_err_no);
				nob_return_defer(false);
			}
			n -= m;
			buff2 += m;
		}
	}
defer:
	NOB_FREE(buff);
	close(src_fd);
	close(dst_fd);
	return result;
#endif
}

void nob_cmd_render(NOB_Cmd cmd, NOB_String_Builder *render) {
	for (size_t i = 0; i < cmd.count; i++) {
		const char *arg = cmd.items[i];
		if (arg == NULL) { break; }
		if (i > 0) { nob_sb_append_cstr(render, " "); }
		if (!strchr(arg, ' ')) {
			nob_sb_append_cstr(render, arg);
		} else {
			nob_da_append(render, '\'');
			nob_sb_append_cstr(render, arg);
			nob_da_append(render, '\'');
		}
	}
}

NOB_Proc nob_cmd_run_async(NOB_Cmd cmd) {
	if (cmd.count < 1) {
		nob_log(NOB_ERROR, "Could not run empty command");
		return NOB_INVALID_PROC;
	}
	NOB_String_Builder sb = {0};
	nob_cmd_render(cmd, &sb);
	nob_sb_append_null(&sb);
	nob_log(NOB_INFO, "CMD: %s", sb.items);
	nob_sb_free(sb);
	memset(&sb, 0, sizeof(sb));
#ifdef _WIN32
	STARTUPINFO siStartInfo;
	ZeroMemory(&siStartInfo, sizeof(siStartInfo));
	siStartInfo.cb = sizeof(STARTUPINFO);

	siStartInfo.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
	siStartInfo.hStdOutput = GetStdHandle(STD_ERROR_OUTPUT);
	siStartInfo.hStdInput  = GetStdHandle(STD_ERROR_INPUT);
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION piProcInfo;
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	nob_cmd_render(cmd, &sb);
	nob_sb_append_null(&sb);
	BOOL bSuccess = CreateProcessA(NULL, sb.items, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
	nob_sb_free(sb);
	if (!bSuccess) {
		nob_log(NOB_ERROR, "Could not create a child process: %lu", GetLastError());
		return NOB_INVALID_PROC;
	}
	CloseHandle(piProcInfo.hThread);

	return piProcInfo.hProcess;
#else
	pid_t cpid = fork();
	if (cpid < 0) {
		nob_log(NOB_ERROR, "Could not fork child process: %s", str_err_no);
		return NOB_INVALID_PROC;
	}
	if (cpid == 0) {
		NOB_Cmd cmd_null = {0};
		nob_da_append_many(&cmd_null, cmd.items, cmd.count);
		nob_da_append(&cmd_null, NULL);

		if (execvp(cmd.items[0], (char *const *)cmd_null.items) < 0) {
			nob_log(NOB_ERROR, "Could not exec child process: %s", str_err_no);
			exit(1);
		}
		NOB_ASSERT(0 && "unreachable");
	}
	return cpid;
#endif
}

bool nob_procs_wait(NOB_Procs procs) {
	for (size_t i = 0; i < procs.count; i++) {
		if (!nob_proc_wait(procs.items[i])) { return false; }
	}
	return true;
}

bool nob_proc_wait(NOB_Proc proc) {
	if (proc == NOB_INVALID_PROC) { return false; }
#ifdef _WIN32
	DWORD result = WaitForSignalObject(
		proc,	 // handle hHandle
		INFINITE // DWORD dwMilliseconds
	);

	if (result == WAIT_FAILED) {
		nob_log(NOB_ERROR, "could not wait on child process: %lu", GetLastError());
		return false;
	}

	DWORD exit_status;
	if (!GetExitCodeProcess(proc, &exit_status)) {
		nob_log(NOB_ERROR, "could not get process exit code %lu", GetLastError());
		return false;
	}

	if (exit_status != 0) {
		nob_log(NOB_ERROR, "command exited with exit code %lu", exit_status);
		return false
	}

	CloseHandle(proc);

	return true;
#else
	while (1) {
		int wstatus = 0;
		if (waitpid(proc, &wstatus, 0) < 0) {
			nob_log(NOB_ERROR, "could not wait on command (pid %d): %s", proc, str_err_no);
			return false;
		}

		if (WIFEXITED(wstatus)) {
			int exit_status = WEXITSTATUS(wstatus);
			if (exit_status != 0) {
				nob_log(NOB_ERROR, "command exited with exit code %d", exit_status);
				return false;
			}
			break;
		}
		if (WIFSIGNALED(wstatus)) {
			nob_log(NOB_ERROR, "command process was terminated by %s", strsignal(WTERMSIG(wstatus)));
			return false;
		}
	}
	return true;
#endif
}

bool nob_cmd_run_sync(NOB_Cmd cmd) {
	NOB_Proc p = nob_cmd_run_async(cmd);
	if (p == NOB_INVALID_PROC) { return false; }
	return nob_proc_wait(p);
}

char *nob_shift_args(int *argc, char ***argv) {
	NOB_ASSERT(*argc > 0);
	char *result = **argv;
	(*argv)++;
	(*argc)--;
	return result;
}

void nob_log(NOB_Log_Level level, const char *fmt, ...) {
	switch (level) {
	case NOB_INFO: {
		fprintf(stderr, "[INFO] ");
	} break;
	case NOB_WARNING: {
		fprintf(stderr, "[WARNING] ");
	} break;
	case NOB_ERROR: {
		fprintf(stderr, "[ERROR] ");
	} break;
	default: {
		NOB_ASSERT(0 && "unreachable");
	}
	}
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, NOB_LINE_END);
}

bool nob_read_entire_dir(const char *parent, NOB_File_Paths *children) {
	bool result = true;
	DIR *dir	= NULL;

	dir = opendir(parent);
	if (dir == NULL) {
		nob_log(NOB_ERROR, "Could not open directory %s: %s", parent, str_err_no);
		nob_return_defer(false);
	}
	errno			   = 0;
	struct dirent *ent = readdir(dir);
	while (ent != NULL) {
		nob_da_append(children, nob_temp_strdup(ent->d_name));
		ent = readdir(dir);
	}
	if (errno != 0) {
		nob_log(NOB_ERROR, "Could not read directory %d: %s", parent, str_err_no);
		nob_return_defer(false);
	}
defer:
	if (dir) { closedir(dir); }
	return result;
}

bool nob_write_entire_file(const char *path, const void *data, size_t size) {
	bool  result   = 0;
	FILE *out_file = fopen(path, "wb");
	if (out_file == NULL) {
		nob_log(NOB_ERROR, "could not open file %s for writing: %s", path, str_err_no);
		nob_return_defer(false);
	}

	const char *buff = data;
	while (size > 0) {
		size_t n = fwrite(buff, 1, size, out_file);
		if (ferror(out_file)) {
			nob_log(NOB_ERROR, "could not write to file %s: %s", path, str_err_no);
			nob_return_defer(false);
		}
		size -= n;
		buff += n;
	}
defer:
	if (out_file) { fclose(out_file); }
	return result;
}

NOB_File_Type nob_get_file_type(const char *path) {
#ifdef _WIN32
	DWORD attr = GetFileAttributes(path);
	if (attr == INVALID_FILE_ATTRIBUTES) {
		nob_log(NOB_ERROR, "Could not get file attributes of %s: %lu", path, GetLastError());
		return -1;
	}
	if (attr & FILE_ATTRIBUTES_DIRECTORY) return NOB_FILE_DIRECTORY;
	// TODO: detect symlinks on windows
	return NOB_FILE_REGULAR;
#else
	struct stat statbuf;
	if (stat(path, &statbuf) < 0) {
		nob_log(NOB_ERROR, "Could not get stat of %s: %s", path, str_err_no);
		return -1;
	}
	switch (statbuf.st_mode & __S_IFMT) {
	case __S_IFDIR: {
		return NOB_FILE_DIRECTORY;
	}
	case __S_IFREG: {
		return NOB_FILE_REGULAR;
	}
	case __S_IFLNK: {
		return NOB_FILE_SYMLINK;
	}
	default: {
		return NOB_FILE_OTHER;
	}
	}
#endif
}

bool nob_copy_directory_recursively(const char *src_path, const char *dst_path) {
	bool			   result		   = true;
	NOB_File_Paths	   children		   = {0};
	NOB_String_Builder src_sb		   = {0};
	NOB_String_Builder dst_sb		   = {0};
	size_t			   temp_checkpoint = nob_temp_save();

	NOB_File_Type type = nob_get_file_type(src_path);
	if (type < 0) { return false; }

	switch (type) {
	case NOB_FILE_DIRECTORY: {
		if (!nob_mkdir_if_not_exists(dst_path)) nob_return_defer(false);
		if (!nob_read_entire_dir(src_path, &children)) nob_return_defer(false);
		for (size_t i = 0; i < children.count; i++) {
			if (strcmp(children.items[i], ".") == 0) { continue; }
			if (strcmp(children.items[i], "..") == 0) { continue; }

			src_sb.count = 0;
			nob_sb_append_cstr(&src_sb, src_path);
			nob_sb_append_cstr(&src_sb, "/");
			nob_sb_append_cstr(&src_sb, children.items[i]);
			nob_sb_append_null(&src_sb);

			if (!nob_copy_directory_recursively(src_sb.items, dst_sb.items)) { nob_return_defer(false); }
		}
	} break;
	case NOB_FILE_REGULAR: {
		if (!nob_copy_file(src_path, dst_path)) { nob_return_defer(false); }
	} break;
	case NOB_FILE_SYMLINK: {
		nob_log(NOB_WARNING, "TODO: Copying symlinks is not supported yet");
	} break;
	case NOB_FILE_OTHER: {
		nob_log(NOB_ERROR, "Unsupported type of file %s", src_path);
		nob_return_defer(false);
	} break;
	default: {
		NOB_ASSERT(0 && "unreachable");
	}
	}
defer:
	nob_temp_rewind(temp_checkpoint);
	nob_da_free(src_sb);
	nob_da_free(dst_sb);
	nob_da_free(children);
	return result;
}

char *nob_temp_strdup(const char *cstr) {
	size_t n	  = strlen(cstr);
	char	 *result = nob_temp_alloc(n + 1);
	NOB_ASSERT(result != NULL && "Increase NOB_TEMP_CAPACITY");
	memcpy(result, cstr, n);
	result[n] = 0;
	return result;
}

void *nob_temp_alloc(size_t size) {
	if (nob_temp_size + size > NOB_TEMP_CAPACITY) { return NULL; }
	void *result = &nob_temp[nob_temp_size];
	nob_temp_size += size;
	return result;
}

char *nob_temp_sprintf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int n = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	NOB_ASSERT(n >= 0);
	char *result = nob_temp_alloc(n + 1);
	NOB_ASSERT(result != NULL && "Extend the size of the temporary allocator");
	// TODO: use proper arenas for the temporary allocator;
	va_start(args, fmt);
	vsnprintf(result, n + 1, fmt, args);
	va_end(args);

	return result;
}

void nob_temp_reset(void) { nob_temp_size = 0; }

size_t nob_temp_save(void) { return nob_temp_size; }

void nob_temp_rewind(size_t checkpoint) { nob_temp_size = checkpoint; }

const char *nob_temp_sv_to_cstr(NOB_String_View sv) {
	char *result = nob_temp_alloc(sv.count + 1);
	NOB_ASSERT(result != NULL && "Extend the size of the temporary allocator");
	memcpy(result, sv.data, sv.count);
	result[sv.count] = 0;
	return result;
}

int nob_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count) {
#ifdef _WIN32
	BOOL bSuccess;

	HANDLE output_path_fd = CreateFile(output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	if (output_fd == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) { return 1; }
		nob_log(NOB_ERROR, "Could not open file %s: %lu", output_path, GetLastError());
		return -1;
	}
	FILETIME output_path_time;
	bSuccess = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
	CloseHandle(output_path_fd);
	if (!bSuccess) {
		nob_log(NOB_ERROR, "Could not get time of %s: %lu", output_path, GetLastError());
		return -1;
	}

	for (size_t i = 0; i < input_paths_count; i++) {
		const char *input_path	  = input_paths[i];
		HANDLE		input_path_fd = CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
		if (input_path_fd == INVALID_HANDLE_VALUE) {
			nob_log(NOB_ERROR, "Could not open file: %s: %lu", input_path, GetLastError());
			return -1;
		}
		FILETIME input_path_time;
		bSuccess = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
		CloseHandle(input_path_fd);
		if (!bSuccess) {
			nob_log(NOB_ERROR, "Could not get time of %s: %lu", input_path, GetLastError());
			return -1;
		}
		if (CompareFileTime(&input_path_time, &output_path_time) == 1) { return 1; }
	}
	return 0;
#else
	struct stat statbuf = {0};

	if (stat(output_path, &statbuf) < 0) {
		if (errno == ENOENT) { return 1; }
		nob_log(NOB_ERROR, "could not stat %s: %s", output_path, str_err_no);
		return -1;
	}
	int output_path_time = statbuf.st_mtime;

	for (size_t i = 0; i < input_paths_count; i++) {
		const char *input_path = input_paths[i];
		if (stat(input_path, &statbuf) < 0) {
			nob_log(NOB_ERROR, "could not stat %s: %s", input_path, str_err_no);
			return -1;
		}
		int input_path_time = statbuf.st_mtime;
		if (input_path_time > output_path_time) { return 1; }
	}
#endif
	return 0;
}

int nob_needs_rebuild1(const char *output_path, const char *input_path) {
	return nob_needs_rebuild(output_path, &input_path, 1);
}

bool nob_rename(const char *old_path, const char *new_path) {
	nob_log(NOB_INFO, "renaming %s -> %s", old_path, new_path);
#ifdef _WIN32
	if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING)) {
		nob_log(NOB_ERROR, "could not rename %s to %s: %lu", old_path, new_path, GetLastError());
		return false;
	}
#else
	if (rename(old_path, new_path) < 0) {
		nob_log(NOB_ERROR, "could not rename %s to %s: %s", old_path, new_path, str_err_no);
		return false;
	}
#endif
	return true;
}

bool nob_read_entire_file(const char *path, NOB_String_Builder *sb) {
	bool  result	 = true;
	FILE *input_file = fopen(path, "rb");
	if (input_file == NULL) { nob_return_defer(false); }
	if (fseek(input_file, 0, SEEK_END) < 0) { nob_return_defer(false); }
	long m = ftell(input_file);
	if (m < 0) { nob_return_defer(false); }
	if (fseek(input_file, 0, SEEK_SET) < 0) { nob_return_defer(false); }

	size_t new_count = sb->count + m;
	if (new_count > sb->capacity) {
		sb->items = NOB_REALLOC(sb->items, new_count);
		NOB_ASSERT(sb->items != NULL && "Buy More RAM LOL");
		sb->capacity = new_count;
	}
	fread(sb->items + sb->count, m, 1, input_file);
	if (ferror(input_file)) { nob_return_defer(false); }
	sb->count = new_count;
defer:
	if (!result) { nob_log(NOB_ERROR, "Could not read file %s: %s", path, str_err_no); }
	if (input_file) { fclose(input_file); }
	return result;
}

NOB_String_View nob_sv_chop_by_delim(NOB_String_View *sv, char delim) {
	size_t i = 0;
	while (i < sv->count && sv->data[i] != delim) { i++; }
	NOB_String_View result = nob_sv_from_parts(sv->data, i);

	if (i < sv->count) {
		sv->count -= i + 1;
		sv->data += i + 1;
	} else {
		sv->count -= i;
		sv->data += i;
	}
	return result;
}

NOB_String_View nob_sv_from_parts(const char *data, size_t count) {
	NOB_String_View sv = {0};
	sv.count		   = count;
	sv.data			   = data;
	return sv;
}

NOB_String_View nob_sv_trim_left(NOB_String_View sv) {
	size_t i = 0;
	while (i < sv.count && isspace(sv.data[i])) { i++; }
	return nob_sv_from_parts(sv.data + i, sv.count - i);
}

NOB_String_View nob_sv_trim_right(NOB_String_View sv) {
	size_t i = 0;
	while (i < sv.count && isspace(sv.data[sv.count - i - 1])) { i++; }
	return nob_sv_from_parts(sv.data, sv.count - i);
}

NOB_String_View nob_sv_trim(NOB_String_View sv) { return nob_sv_trim_right(nob_sv_trim_left(sv)); }

NOB_String_View nob_sv_from_cstr(const char *cstr) { return nob_sv_from_parts(cstr, strlen(cstr)); }

bool nob_sv_eq(NOB_String_View a, NOB_String_View b) { return (a.count == b.count) && (memcmp(a.data, b.data, a.count) == 0); }

/**
 * @brief tells you whether a file exists or not
 * @param file_path
 * @return  0 - file does not exist;
 * @return  1 - file exists;
 * @return -1 - error while checking the file, error logged;
 */
int nob_file_exists(const char *file_path) {
#ifdef _WIN32
	// TODO: distinguish between "does not exist" and other errors
	DWORD dwAttrib = GetFileAttributes(file_path);
	return dwAttrib != INVALID_FILE_ATTRIBUTES;
#else
	struct stat statbuf;
	if (stat(file_path, &statbuf) < 0) {
		if (errno == ENOENT) { return 0; }
		nob_log(NOB_ERROR, "Could not check if file %s exists: %s", file_path, str_err_no);
		return -1;
	}
	return 1;
#endif
}

#ifdef _WIN32
struct DIR {
	HANDLE			hFind;
	WIN32_FIND_DATA data;
	struct dirent  *dirent;
};

DIR *opendir(const char *dirpath) {
	NOB_ASSERT(dirpath);

	char buffer[MAX_PATH];
	snprintf(buffer, MAX_PATH, "%s\\*", dirpath);

	DIR *dir = (DIR *)calloc(1, sizeof(DIR));

	dir->hFind = FindFirstFile(buffer, &dir->data);
	if (dir->hFind == INVALID_HANDLE_VALUE) {
		// TODO: opendir should set errno accordingly on FindFirstFile fail
		errno = ENOSYS;
		goto fail;
	}
	return dir;

fail:
	if (dir) { NOB_FREE(dir); }
	return NULL;
}

struct dirent *readdir(DIR *dirp) {
	NOB_ASSERT(dirp);
	if (dirp->dirent == NULL) {
		dirp->dirent = (struct dirent *)calloc(1, sizeof(struct dirent));
	} else {
		if (!FindNextFile(dirp->hFind, &dirp->data)) {
			// TODO: readdir should set errno accordingly on FindNextFile fail
			errno = ENOSYS;
		}
		return NULL;
	}
	memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));

	strncpy(dirp->dirent->d_name, dirp->data.cFileName, sizeof(dirp->dirent->d_name) - 1);

	return dirp->dirent;
}

int closedir(DIR *dirp) {
	NOB_ASSERT(dirp);

	if (!FindClose(dirp->hFind)) {
		// TODO: closedir should set errno accordingly on FindClose fail
		errno = ENOSYS;
		return -1;
	}
	if (dirp->dirent) { NOB_FREE(dirp->dirent); }
	free(dirp);

	return 0;
}
#endif

#endif
#endif