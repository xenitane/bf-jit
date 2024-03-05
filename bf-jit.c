#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define NOB_IMPL
#include "nob.h"

typedef enum {
	OP_INC			   = '+', // increase the current byte by 1
	OP_DEC			   = '-', // decrease the current byte by 1
	OP_LEFT			   = '<', // move the pointer to the perv byte (can't go left of zero)
	OP_RIGHT		   = '>', // move the pointer to the next byte (can't go right of End of Memory)
	OP_OUTPUT		   = '.', // print the current byte to stdout
	OP_INPUT		   = ',', // read a byte from stdin into current byte
	OP_JMP_IF_ZERO	   = '[', // jumps to instruction next to corresponding ']' if current byte is 0
	OP_JMP_IF_NON_ZERO = ']', // jumps to instruction next to corresponding '[' if current byte in not 0
} Operator_Kind;

typedef struct {
	Operator_Kind op_kind;
	size_t		  operand;
} Operator;

typedef struct {
	Operator *items;
	size_t	  count;
	size_t	  capacity;
} Program;

typedef struct {
	NOB_String_View content;
	size_t			pos;
} Lexer;

bool is_bf_token(char ch) {
	const char *cmds = "+-<>.,[]";
	return strchr(cmds, ch);
}

char lexer_next(Lexer *l, bool (*token_validator)(char)) {
	while (l->pos < l->content.count && !token_validator(l->content.data[l->pos])) l->pos++;
	if (l->pos >= l->content.count) return 0;
	return l->content.data[l->pos++];
}

typedef struct {
	size_t *items;
	size_t	count;
	size_t	capacity;
} Address_Stack;

bool interpret(Program program, size_t memory_size, bool jump_on_boundary) {
	char *memory = calloc(memory_size, 1);
	NOB_ASSERT(memory != NULL && "Buy More RAM LOL");

	size_t head = 0;
	size_t ip	= 0;
	while (ip < program.count) {
		Operator *op = program.items + ip;
		switch (op->op_kind) {
		case OP_INC: {
			memory[head] += op->operand & 0xff;
			ip++;
		} break;
		case OP_DEC: {
			memory[head] -= op->operand & 0xff;
			ip++;
		} break;
		case OP_LEFT: {
			head = (head >= op->operand)
					   ? (head - op->operand)
					   : (jump_on_boundary ? (head + (memory_size - (op->operand % memory_size)) % memory_size) : 0);
			ip++;
		} break;
		case OP_RIGHT: {
			head = (head + op->operand < memory_size)
					   ? (head + op->operand)
					   : (jump_on_boundary ? ((head + (op->operand % memory_size)) % memory_size) : (memory_size - 1));
			ip++;
		} break;
		case OP_INPUT: {
			for (size_t i = 0; i < op->operand; i++) { fread(memory + head, 1, 1, stdin); }
			ip++;
		} break;
		case OP_OUTPUT: {
			for (size_t i = 0; i < op->operand; i++) { fwrite(memory + head, 1, 1, stdout); }
			ip++;
		} break;
		case OP_JMP_IF_ZERO: {
			ip = memory[head] ? (ip + 1) : (op->operand);
		} break;
		case OP_JMP_IF_NON_ZERO: {
			ip = memory[head] ? (op->operand) : (ip + 1);
		} break;
		default: NOB_ASSERT(0 && "unreachable");
		}
	}
	NOB_FREE(memory);
	return true;
}

typedef struct {
	int (*exec)(void *memory);
	size_t len;
} Code;

bool is_valid_code(Code code) { return code.exec != NULL; }

int free_code(Code code) { return munmap(code.exec, code.len); }

typedef struct {
	size_t operand_byte_address;
	size_t src_byte_address;
	size_t dest_op_index;
} BackPatch;

typedef struct {
	size_t	   count;
	size_t	   capacity;
	BackPatch *items;
} BackPatches;

bool compile_to_machine_code(Program program, size_t memory_size, bool jump_on_boundary, Code *code) {
	NOB_String_Builder program_as_machine_code = {0};
	BackPatches		   back_patches			   = {0};
	Address_Stack	   address_stack		   = {0};

	nob_da_append_many(&program_as_machine_code, "\x4D\x31\xD2", 3); // xor r10, r10
	nob_da_append_many(&program_as_machine_code, "\x49\xB8", 2);	 // mov r8,
	nob_da_append_many(&program_as_machine_code, &memory_size, 8);	 // memory_size

	for (size_t i = 0; i < program.count; i++) {
		Operator *op = program.items + i;
		nob_da_append(&address_stack, program_as_machine_code.count);
		switch (op->op_kind) {
		case OP_INC: {
			nob_da_append_many(&program_as_machine_code, "\x48\xB8", 2);	 // mov rax,
			nob_da_append_many(&program_as_machine_code, &(op->operand), 8); // operand
			nob_da_append_many(&program_as_machine_code, "\x00\x07", 2);	 // add byte[rdi], al
		} break;
		case OP_DEC: {
			nob_da_append_many(&program_as_machine_code, "\x48\xB8", 2);	 // mov rax,
			nob_da_append_many(&program_as_machine_code, &(op->operand), 8); // operand
			nob_da_append_many(&program_as_machine_code, "\x28\x07", 2);	 // sub byte[rdi], al
		} break;
		case OP_LEFT: {
			nob_da_append_many(&program_as_machine_code, "\x48\xB8", 2);	 // mov rax,
			nob_da_append_many(&program_as_machine_code, &(op->operand), 8); // operand
			if (jump_on_boundary) {
				nob_da_append_many(&program_as_machine_code, "\x48\x31\xD2", 3); // xor rdx, rdx
				nob_da_append_many(&program_as_machine_code, "\x49\xF7\xF0", 3); // div r8
				nob_da_append_many(&program_as_machine_code, "\x48\x89\xD0", 0); // mov rax, rdx
			}
			nob_da_append_many(&program_as_machine_code, "\x49\x39\xC2", 3); // cmp r10, rax
			nob_da_append_many(&program_as_machine_code, "\x0f\x89", 2);	 // jns
			if (jump_on_boundary) {
				nob_da_append_many(&program_as_machine_code, "\x06\x00\x00\x00", 4); // 6
				nob_da_append_many(&program_as_machine_code, "\x4C\x01\xC7", 3);	 // add rdi, r8
				nob_da_append_many(&program_as_machine_code, "\x4D\x01\xC2", 3);	 // add r10, r8
			} else {
				nob_da_append_many(&program_as_machine_code, "\x03\x00\x00\x00", 4); // 3
				nob_da_append_many(&program_as_machine_code, "\x4C\x89\xD0", 3);	 // mov rax, r10
			}
			nob_da_append_many(&program_as_machine_code, "\x49\x29\xC2", 3); // sub r10, rax
			nob_da_append_many(&program_as_machine_code, "\x48\x29\xC7", 3); // sub rdi, rax
		} break;
		case OP_RIGHT: {
			nob_da_append_many(&program_as_machine_code, "\x48\xB8", 2);	 // mov rax,
			nob_da_append_many(&program_as_machine_code, &(op->operand), 8); // operand
			nob_da_append_many(&program_as_machine_code, "\x48\x31\xD2", 3); // xor rdx, rdx
			nob_da_append_many(&program_as_machine_code, "\x49\xF7\xF0", 3); // div r8
			nob_da_append_many(&program_as_machine_code, "\x48\x89\xD0", 3); // mov rax, rdx
			nob_da_append_many(&program_as_machine_code, "\x4C\x01\xD2", 3); // add rdx, r10
			nob_da_append_many(&program_as_machine_code, "\x4C\x39\xC2", 3); // cmp rdx, r8
			nob_da_append_many(&program_as_machine_code, "\x0f\x88", 2);	 // js
			if (jump_on_boundary) {
				nob_da_append_many(&program_as_machine_code, "\x06\x00\x00\x00", 4); // 6
				nob_da_append_many(&program_as_machine_code, "\x4D\x29\xC2", 3);	 // sub r10, r8
				nob_da_append_many(&program_as_machine_code, "\x4D\x29\xC7", 3);	 // sub rdi, r8
			} else {
				nob_da_append_many(&program_as_machine_code, "\x0c\x00\x00\x00", 4);		 // 9
				nob_da_append_many(&program_as_machine_code, "\x4C\x29\xC2", 3);			 // sub rdx, r8
				nob_da_append_many(&program_as_machine_code, "\x48\x29\xD0", 3);			 // sub rax, rdx
				nob_da_append_many(&program_as_machine_code, "\x48\x2D\x01\x00\x00\x00", 6); // sub rax, 1
			}
			nob_da_append_many(&program_as_machine_code, "\x49\x01\xC2", 3); // add r10, rax
			nob_da_append_many(&program_as_machine_code, "\x48\x01\xC7", 3); // add rdi, rax
		} break;
		case OP_INPUT: {
			nob_da_append_many(&program_as_machine_code, "\x57", 1);						 // push rdi
			nob_da_append_many(&program_as_machine_code, "\x48\xC7\xC0\x00\x00\x00\x00", 7); // mov rax, 0
			nob_da_append_many(&program_as_machine_code, "\x48\x89\xFE", 3);				 // mov rsi, rdi
			nob_da_append_many(&program_as_machine_code, "\x48\xC7\xC7\x00\x00\x00\x00", 7); // mov rdi, 0
			nob_da_append_many(&program_as_machine_code, "\x48\xC7\xC2\x01\x00\x00\x00", 7); // mov rdx, 1
			for (size_t i = 0; i < op->operand; ++i) {
				nob_da_append_many(&program_as_machine_code, "\x0F\x05", 2); // syscall
			}
			nob_da_append_many(&program_as_machine_code, "\x5F", 1); // pop rdi
		} break;
		case OP_OUTPUT: {
			nob_da_append_many(&program_as_machine_code, "\x57", 1);						 // push rdi
			nob_da_append_many(&program_as_machine_code, "\x48\xC7\xC0\x01\x00\x00\x00", 7); // mov rax, 1
			nob_da_append_many(&program_as_machine_code, "\x48\x89\xFE", 3);				 // mov rsi, rdi
			nob_da_append_many(&program_as_machine_code, "\x48\xC7\xC7\x01\x00\x00\x00", 7); // mov rdi, 1
			nob_da_append_many(&program_as_machine_code, "\x48\xC7\xC2\x01\x00\x00\x00", 7); // mov rdx, 1
			for (size_t i = 0; i < op->operand; ++i) {
				nob_da_append_many(&program_as_machine_code, "\x0F\x05", 2); // syscall
			}
			nob_da_append_many(&program_as_machine_code, "\x5F", 1); // pop rdi
		} break;
		case OP_JMP_IF_ZERO: {
			nob_da_append_many(&program_as_machine_code, "\x8A\x07", 2); // mov al, byte[rdi]
			nob_da_append_many(&program_as_machine_code, "\x84\xC0", 2); // test al, al
			nob_da_append_many(&program_as_machine_code, "\x0F\x84", 2); // jz
			size_t operand_byte_addr = program_as_machine_code.count;
			nob_da_append_many(&program_as_machine_code, "\x00\x00\x00\x00", 4); // operand
			BackPatch bp = {
				.operand_byte_address = operand_byte_addr,
				.src_byte_address	  = program_as_machine_code.count,
				.dest_op_index		  = op->operand,
			};
			nob_da_append(&back_patches, bp);
		} break;
		case OP_JMP_IF_NON_ZERO: {
			nob_da_append_many(&program_as_machine_code, "\x8A\x07", 2); // mov al, byte[rdi]
			nob_da_append_many(&program_as_machine_code, "\x84\xC0", 2); // test al, al
			nob_da_append_many(&program_as_machine_code, "\x0F\x85", 2); // jnz
			size_t operand_byte_addr = program_as_machine_code.count;
			nob_da_append_many(&program_as_machine_code, "\x00\x00\x00\x00", 4); // operand

			BackPatch bp = {
				.operand_byte_address = operand_byte_addr,
				.src_byte_address	  = program_as_machine_code.count,
				.dest_op_index		  = op->operand,
			};
			nob_da_append(&back_patches, bp);
		} break;
		default: NOB_ASSERT(0 && "Unreachable");
		}
		//
	}

	nob_da_append(&address_stack, program_as_machine_code.count);

	for (size_t i = 0; i < back_patches.count; i++) {
		BackPatch *bp			= back_patches.items + i;
		int32_t	   src_address	= bp->src_byte_address;
		int32_t	   dest_address = address_stack.items[bp->dest_op_index];
		int32_t	   operand		= dest_address - src_address;
		memcpy(program_as_machine_code.items + bp->operand_byte_address, &operand, sizeof(operand));
	}

	nob_da_append_many(&program_as_machine_code, "\x48\x31\xC0", 3); // xor rax, rax
	nob_sb_append_cstr(&program_as_machine_code, "\xC3");			 // ret

	bool result = true;
	code->exec	= mmap(NULL, program_as_machine_code.count, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	code->len	= program_as_machine_code.count;

	if (code->exec == MAP_FAILED) {
		nob_log(NOB_ERROR, "Could not allocate executable memory: %s", str_err_no);
		nob_return_defer(false);
	}
	memcpy(code->exec, program_as_machine_code.items, program_as_machine_code.count);

defer:
	if (!result) {
		free_code(*code);
		memset(code, 0, sizeof(*code));
	}
	nob_da_free(program_as_machine_code);
	nob_da_free(address_stack);
	nob_da_free(back_patches);
	return result;
}

bool all_digits(NOB_String_View sv) {
	while (sv.count--) {
		if (!isdigit(sv.data[sv.count])) return false;
	}
	return true;
}

bool handle_args(int *argc, char ***argv, const char **file_path, size_t *memory_size, bool *jit_mode, bool *jump_on_boundary) {
	int			result = true;
	const char *binary = nob_shift_args(argc, argv);

	if (*argc <= 0) { nob_return_defer(0); }

	bool jit_arg = false;
	bool mem_arg = false;
	bool jmp_arg = false;
	bool fil_arg = false;

	while (*argc) {
		const char *arg = nob_shift_args(argc, argv);
		if (!strcmp("--no-jit", arg)) {
			if (jit_arg) {
				nob_log(NOB_ERROR, "Use an option once");
				nob_return_defer(0);
			}
			jit_arg	  = true;
			*jit_mode = false;
			continue;
		}
		if (!strcmp("--jmp-bnd", arg)) {
			if (jmp_arg) {
				nob_log(NOB_ERROR, "Use an option once");
				nob_return_defer(0);
			}
			jmp_arg			  = true;
			*jump_on_boundary = true;
			continue;
		}
		if (strlen(arg) > 4 && nob_sv_eq(nob_sv_from_parts(arg, 4), nob_sv_from_cstr("--M="))) {
			if (!all_digits(nob_sv_from_cstr(arg + 4))) {
				nob_log(NOB_ERROR, "Enter a valid size for memory, aka a positive integer");
				nob_return_defer(0);
			}
			if (mem_arg) {
				nob_log(NOB_ERROR, "Use an option once");
				nob_return_defer(0);
			}
			mem_arg = true;
			char *endptr;
			*memory_size = strtoul(arg + 4, &endptr, 10);
			continue;
		}
		if (nob_file_exists(arg)) {
			if (fil_arg) {
				nob_log(NOB_ERROR, "Can run only one program at a time");
				nob_return_defer(0);
			}
			fil_arg	   = true;
			*file_path = arg;
			continue;
		}
		nob_log(NOB_ERROR, "Invalid argument %s", arg);
		nob_return_defer(0);
	}
defer:
	if (!result || !fil_arg) {
		nob_log(NOB_ERROR, "Usage: %s [OPTIONS] <input.bf>", binary);
		nob_log(NOB_ERROR, "No Input Is Provided");
		nob_log(NOB_ERROR, "Read docs for more help on usage");
	}
	return result;
}

bool string_to_program(const char *file_path, bool jit_mode, Program *program) {
	NOB_String_Builder program_as_string = {0};
	bool			   result			 = true;
	if (!nob_read_entire_file(file_path, &program_as_string)) nob_return_defer(false);

	Lexer l = {
		.content =
			{
				.count = program_as_string.count,
				.data  = program_as_string.items,
			},
		.pos = 0,
	};
	Address_Stack address_stack = {0};
	char		  token			= lexer_next(&l, is_bf_token);
	while (token) {
		switch (token) {
		case OP_INC:
		case OP_DEC:
		case OP_RIGHT:
		case OP_LEFT:
		case OP_OUTPUT:
		case OP_INPUT: {
			size_t count		= 1;
			char   operator_tmp = lexer_next(&l, is_bf_token);
			while ((operator_tmp == token) && jit_mode) {
				count++;
				operator_tmp = lexer_next(&l, is_bf_token);
			}
			Operator op = {
				.op_kind = token,
				.operand = count,
			};
			nob_da_append(program, op);
			token = operator_tmp;
		} break;
		case OP_JMP_IF_ZERO: {
			size_t address = program->count;

			Operator op = {
				.op_kind = token,
				.operand = 0,
			};

			nob_da_append(program, op);
			nob_da_append(&address_stack, address);

			token = lexer_next(&l, is_bf_token);
		} break;
		case OP_JMP_IF_NON_ZERO: {
			if (!address_stack.count) {
				nob_log(NOB_ERROR, "%s [%zu]: Unbalanced jump", file_path, l.pos);
				nob_return_defer(false);
			}

			size_t address = address_stack.items[--address_stack.count];

			Operator op = {
				.op_kind = token,
				.operand = address + 1,
			};

			nob_da_append(program, op);
			program->items[address].operand = program->count;

			token = lexer_next(&l, is_bf_token);

		} break;
		default: NOB_ASSERT(0 && "Unreachable");
		}
	}

	if (address_stack.count) {
		nob_log(NOB_ERROR, "%s: Unbalanced jump", file_path);
		nob_return_defer(false);
	}

defer:
	if (!result) {
		nob_da_free(*program);
		memset(program, 0, sizeof(*program));
	}
	return result;
}

int main(int argc, char **argv) {

	// TODO: handle multiple files | execute them sequentially and mode 1: preserve and share the state of memory, mode 2: start
	// with clean memory
	const char *file_path		 = NULL;
	size_t		memory_size		 = 10000;
	bool		jit_mode		 = true;
	bool		jump_on_boundary = false;

	int		result		 = 0;
	Program program		 = {0};
	Code	program_code = {0};
	void	 *memory		 = NULL;

	if (!handle_args(&argc, &argv, &file_path, &memory_size, &jit_mode, &jump_on_boundary)) { nob_return_defer(1); }
	if (!string_to_program(file_path, jit_mode, &program)) { nob_return_defer(1); }
	// TODO: switch b/w interpret mode and compile mode with a flag
	// if (!interpret(program, memory_size, jump_on_boundary)) { nob_return_defer(1); }
	if (!compile_to_machine_code(program, memory_size, jump_on_boundary, &program_code)) { nob_return_defer(1); }
	if (!is_valid_code(program_code)) { nob_return_defer(1); }

	memory = calloc(memory_size, 1);
	NOB_ASSERT(memory != NULL && "Buy More RAM lol");
	program_code.exec(memory);

defer:

	nob_da_free(program);
	free_code(program_code);
	NOB_FREE(memory);

	return result;
}

// TODO: add optimization for specific cases like [-] can be read as memory[head]=0 etc
// TODO: make it cross platform i.e. windows (sucks)