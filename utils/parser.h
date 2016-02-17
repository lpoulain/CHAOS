#ifndef __PARSE_H
#define __PARSE_H

#define PARSE_UNDEFINED	0
#define PARSE_WORD		1
#define PARSE_NUMBER	2
#define PARSE_PLUS		3
#define PARSE_MINUS		4
#define PARSE_MULT		5
#define PARSE_DIV		6
#define PARSE_CMD		7
#define PARSE_PARENTHESE_OPEN	8
#define PARSE_PARENTHESE_CLOSE	9

typedef struct token_s {
	uint code;
	void *value;
	uint position;
	struct token_s *next;
} Token;

uint parse(char *cmd, Token **);
int is_math_formula(Token *start, Token *end, int *value);
void parser_print_tokens();
void parser_memory_cleanup();

#endif
