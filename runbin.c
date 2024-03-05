#include <sys/mman.h>

#define NOB_IMPL
#include "nob.h"

typedef void (*code_t)(char *);

int main(int argc, char **argv) {
	const char *program = nob_shift_args(&argc, &argv);

	if (argc < 1) {
		nob_log(NOB_ERROR, "Usage: %s <input.bin>", program);
		nob_log(NOB_ERROR, "No input binary provided");
		return 1;
	}

	const char *file_path = nob_shift_args(&argc, &argv);

	NOB_String_Builder input_file_sb = {0};

	if (!nob_read_entire_file(file_path, &input_file_sb)) return 1;

	code_t code = mmap(NULL, input_file_sb.count, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

	if (code == MAP_FAILED) {
		nob_log(NOB_ERROR, "Could not allocate executable memory: %s", str_err_no);
		return 1;
	}

	memcpy(code, input_file_sb.items, input_file_sb.count);

	char arr[4] = {255, 255, 255, 255};

	code(arr);

	return 0;
}