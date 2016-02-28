#include "libc.h"
#include "display.h"
#include "disk.h"

struct instr_t;

#define ASM_EAX			1
#define ASM_EBX			2
#define ASM_ESP_DERIVE	3
#define ASM_VAL			4
#define ASM_MOV_EAX_X	5
#define ASM_PUSH_EAX	6
#define ASM_PUSH		7
#define ASM_POP_EAX		8
#define ASM_POP_EBX		9
#define ASM_MOV_EAX		10
#define ASM_ADD			11
#define ASM_SUB			12
#define ASM_IMUL_EBX	13
#define ASM_IDIV_EBX	14

typedef struct instr_t {
	uint8 opcode;
	int value;
	struct instr_t *next;
} Instruction;

int compile_math_formula(Token *, Token *, Instruction *);
unsigned char *assemble(Instruction *, unsigned char *);
void print_instructions(Instruction *);
void compile_formula(Window *, const char *, uint, DirEntry *);
