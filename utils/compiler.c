#include "libc.h"
#include "kheap.h"
#include "parser.h"
#include "compiler.h"
#include "process.h"
#include "elf.h"
#include "disk.h"

Instruction *compile(uint8 opcode, int value, Instruction *last) {
	Instruction *result = (Instruction*)kmalloc(sizeof(Instruction));
	last->next = result;

	result->opcode = opcode;
	result->value = value;
	result->next = 0;
	return result;
}

unsigned char *assemble(Instruction *inst, unsigned char *output) {
	unsigned char *val;
	int i;

	switch(inst->opcode) {
		case ASM_MOV_EAX:
			*output++ = 0xB8;
			val = (unsigned char *)&inst->value;
			for (i=0; i<4; i++) *output++ = *val++;
			break;
		case ASM_PUSH:
			*output++ = 0x68;
			val = (unsigned char *)&inst->value;
			for (i=0; i<4; i++) *output++ = *val++;
			break;
		case ASM_PUSH_EAX:
			*output++ = 0x50;
			break;
		case ASM_POP_EAX:
			*output++ = 0x58;
			break;
		case ASM_POP_EBX:
			*output++ = 0x5B;
			break;
		case ASM_ADD:
			*output++ = 0x01;
			*output++ = 0x04;
			*output++ = 0x24;
			break;
		case ASM_SUB:
			*output++ = 0x29;
			*output++ = 0x04;
			*output++ = 0x24;
			break;
		case ASM_IMUL_EBX:
			*output++ = 0xF7;
			*output++ = 0xEB;
			break;
		case ASM_MOV_EAX_X:
			*output++ = 0x89;
			*output++ = 0xC8;
			break;
		case ASM_IDIV_EBX:
			break;
	}

	return output;
}

int compile_math_formula(Token *start, Token *end, Instruction *instructions) {
	// Empty equation => NOT a valid formula
	if (start == end) return 0;

	int val1, val2, res;
	Token *token = start, *old_token1, *old_token2;

	// We need to make sure * and / (major operations) take precedence over
	// + and - (minor operations), no matter what the order
	// For this we consider a formula to be like:
	// val1 * val2 * ... * valn + valn+1 * ... * valm + ...
	//
	// We thus use two accumulators: one for "major operations groups" (e.g. val1 * val2 * .. * valn)
	// and a minor operations accumulator that sums/substracts all the values from the major op groups
	int accum_minor = 0, accum_major = 1, major_op = PARSE_MULT, minor_op = PARSE_PLUS, op=0;
	char value_buffer[10];
	char *value;

	instructions = compile(ASM_PUSH, 0, instructions);
	instructions = compile(ASM_PUSH, 1, instructions);

//	printf("push 0\n");
//	printf("push 1\n");

	while (token != end) {

		// Exmine the token (parenthese or otherwise)

		// The beginning is a valid parenthesis
		if (token->code == PARSE_PARENTHESE_OPEN) {
			int nb_parenth = 1;
			old_token1 = token;
			token = token->next;
			while (nb_parenth > 0 && token != end) {
				if (token->code == PARSE_PARENTHESE_OPEN) nb_parenth++;
				if (token->code == PARSE_PARENTHESE_CLOSE) nb_parenth--;
				if (nb_parenth < 0) {
					error("Too many )'s");
					return -token->position;
				}
				old_token2 = token;
				token = token->next;
			}
			if (nb_parenth > 0) {
				error("Missing a )");
				return -old_token2->position;
			}

			// (...) is not a valid equation
			res = compile_math_formula(old_token1->next, old_token2, instructions);
			if (res <= 0) {
				error("Not a math formula");
				return res;
			}
			while (instructions->next) instructions = instructions->next;
		}

		// The beginning is a number
		else if (token->code == PARSE_NUMBER) {
//			printf("mov eax, %d\n", (int)token->value);
			instructions = compile(ASM_MOV_EAX, (int)token->value, instructions);
			old_token1 = token;
			token = token->next;
		}

		else if (token->code == PARSE_WORD && (!strcmp(token->value, "x"))) {
//			printf("mov eax, x\n");
			instructions = compile(ASM_MOV_EAX_X, 0, instructions);
			old_token1 = token;
			token = token->next;
		}

		// wrong character
		else {
			error("Wrong symbol");
			return -token->position;
		}

		// We have the value, let's mult/div it to the major accumulator
//		printf("pop ebx\n");
//		printf("imul ebx\n");
//		printf("push eax\n");
		instructions = compile(ASM_POP_EBX, 0, instructions);
		instructions = compile(ASM_IMUL_EBX, 0, instructions);
		instructions = compile(ASM_PUSH_EAX, 0, instructions);

		// No more argument. We're done
		if (token == end) {
			// Let's add accum_major to the minor accumulator
//			printf("pop eax\n");
			instructions = compile(ASM_POP_EAX, 0, instructions);

			if (minor_op == PARSE_PLUS) {
//				printf("add [esp], eax\n");
				instructions = compile(ASM_ADD, 0, instructions);
			}
			else {
//				printf("sub [esp], eax\n");
				instructions = compile(ASM_SUB, 0, instructions);
			}
//			printf("pop eax\n");
			instructions = compile(ASM_POP_EAX, 0, instructions);
			// and we have the result
//			value = accum_minor;
			return 1;
		}

		// Check the next symbol is an operation
		if (token->next == end || (token->code != PARSE_PLUS &&
							  token->code != PARSE_MINUS &&
							  token->code != PARSE_MULT &&
							  token->code != PARSE_DIV) ) {
			error("Expected an operation");
			return -token->position;
		}

		op = token->code;

		// We end the series of major operations
		if (op == PARSE_PLUS || op == PARSE_MINUS) {
//			printf("pop eax\n");
			instructions = compile(ASM_POP_EAX, 0, instructions);
			if (minor_op == PARSE_PLUS) {
//				printf("add [esp], eax\n");
				instructions = compile(ASM_ADD, 0, instructions);
			}
			else {
//				printf("sub [esp], eax\n");
				instructions = compile(ASM_SUB, 0, instructions);
			}
			accum_major = 1;
//			printf("push 1\n");
			instructions = compile(ASM_PUSH, 1, instructions);
			major_op = PARSE_MULT;
			minor_op = op;
		}
		else major_op = op;

		old_token1 = token;
		token = token->next;
	}

	error("Formula incomplete");
	return -old_token1->position;
}

void compile_formula(Window *win, const char *filename, uint dir_cluster, DirEntry *dir_index) {
	File f_source;

	disk_ls(dir_cluster, dir_index);
	int result = disk_load_file(filename, dir_cluster, dir_index, &f_source);

	if (result == DISK_ERR_DOES_NOT_EXIST) {
		win->action->puts(win, "The file does not exist");
		win->action->putcr(win);
		return;
	}

	if (result == DISK_ERR_NOT_A_FILE) {
		win->action->puts(win, "This is not a file");
		win->action->putcr(win);
		return;
	}

	if (result != DISK_CMD_OK) {
		win->action->puts(win, "Error");
		win->action->putcr(win);
		return;
	}

	// Parses the content of the file into a series of tokens
	Instruction inst;
	inst.opcode = 0;
	inst.next = 0;

	Token *tokens;

//		printf_win(win, "       %s\n", f_source.body);

	// If there any error, print an error message and quit
	int res = parse(f_source.body, &tokens);

	if (res <= 0) {
		for (int i=0; i<7-res; i++) win->action->putc(win, ' ');
		win->action->putc(win, '^');
		win->action->putcr(win);
		win->action->puts(win, "Syntax error");
		win->action->putcr(win);
		parser_memory_cleanup(tokens);
		return;
	}


//		parser_print_tokens(tokens);

	// Parses and compiles the formula
	// Instead of a chained list of tokens, we now have a chained list of x86 instructions
	res = compile_math_formula(tokens, 0, &inst);
	parser_memory_cleanup(tokens);

	// If this is not a valid formula, print an error message and return
	if (res < 0) {
		for (int i=0; i<7-res; i++) win->action->putc(win, ' ');
		win->action->putc(win, '^');
		win->action->putcr(win);
		if (error_get()[0] != 0) {
			win->action->puts(win, error_get());
			win->action->putcr(win);
		}
		return;
	}		

	// Generate the x86 assembly
	unsigned char buf[512];
	memset(&buf, 0, 512);
	unsigned char *ptr = (unsigned char *)&buf;
	uint buf_size;
	Instruction *in = &inst;

	while (in) {
		ptr = assemble(in, ptr);
		in = in->next;
	}
	buf_size = (uint)ptr - (uint)&buf;
//		printf_win(win, "=> %x (%d bytes)\n", &buf, buf_size);
	//print_instructions(&inst);

	// Loads the previous compiled binary
	int i=0;
	while (filename[i] != '.' && i++ < 256);
	char compiled_filename[256];
	strncpy((char*)&compiled_filename, filename, i);
	compiled_filename[i] = 0;

	Elf *elf = elf_load((char*)&compiled_filename, dir_cluster);

//		printf_win(win, "Source code loaded at: %x\n", f_source.body);
//		printf_win(win, "Compiled binary loaded at: %x\n", elf->header);

	// Inserts the new formula code
	// Technically we should update as well
	unsigned char *code_ptr = elf->section[ELF_SECTION_TEXT].start;
	ptr = code_ptr + 0x18B;
	// mov -0x4(%ebp),%ecx
	*ptr++ = 0x8B;
	*ptr++ = 0x4D;
	*ptr++ = 0xFC;
	// Copy the formula
	memcpy(ptr, buf, 512);
	ptr += buf_size;
	// push %eax
	*ptr++ = 0x50;
	// call print
	*ptr++ = 0xE8;
	int offset = ((int)elf->section[ELF_SECTION_TEXT].start + 0x1F) - ((int)ptr + 4);
	memcpy(ptr, &offset, 4);
	ptr += 4;
	// add $0x4, %esp
	*ptr++ = 0x83;
	*ptr++ = 0xC4;
	*ptr++ = 0x04;
	// nop
	*ptr++ = 0x90;
	// leave
	*ptr++ = 0xC9;
	// ret
	*ptr++ = 0xC3;

	// Write the file to disk
    disk_write_file(elf->file);

    // Free the allocated data
    kfree(elf->header);
    kfree(elf->file);
    kfree(elf);

    Instruction *old_instruction, *instruction = inst.next;
    while (instruction) {
    	old_instruction = instruction;
    	instruction = instruction->next;
    	free(old_instruction);
    }
}
