#include "libc.h"
#include "parser.h"
#include "heap.h"

token tokens[10];
int nb_tokens;

int atoi_substr(char *str, int start, int end)
{
    int res = 0; // Initialize result
  
    // Iterate through all characters of input string and
    // update result
    for (int i = start; i <= end; ++i)
        res = res*10 + str[i] - '0';
  
    // return result.
    return res;
}

#define NEXT_WORD while ( (c == ' ' || c == '\t' ||c == '\n') && c != 0 ) c = cmd[++cmd_pos]

uint parse(char *cmd) {
	int parse_state = PARSE_UNDEFINED;
	int cmd_pos = 0, token_start, token_end;
	nb_tokens = 0;

	char c = cmd[cmd_pos];
	char *word;

	NEXT_WORD;

	while (c != 0) {
		// Number
		if (c >= '0' && c <= '9') {
			token_start = cmd_pos;
			c = cmd[++cmd_pos];
			while (c >= '0' && c <= '9' && c != 0) c = cmd[++cmd_pos];
			tokens[nb_tokens].code = PARSE_NUMBER;
			tokens[nb_tokens++].value = (char*)atoi_substr(cmd, token_start, cmd_pos-1);

			if (c != ' ' && c != '+' && c != '-' && c != '*' && c != '/' && c != '=' && c != 0) return -cmd_pos;
		}
		// Symbol
		else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
			token_start = cmd_pos;
			c = cmd[++cmd_pos];
			while (((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_')) && c != 0) c = cmd[++cmd_pos];

			word = (char *)kmalloc(cmd_pos - token_start, 0);
			strncpy(word, cmd + token_start, cmd_pos - token_start);
			word[cmd_pos - token_start] = 0;

			tokens[nb_tokens].code = PARSE_WORD;
			tokens[nb_tokens++].value = word;

			if (c != ' ' && c != '+' && c != '-' && c != '*' && c != '/' && c != '=' && c != 0) return -cmd_pos;
		}
		// +
		else if (c == '+') {
			tokens[nb_tokens++].code = PARSE_PLUS;
			c = cmd[++cmd_pos];
		}
		else if (c == '-') {
			tokens[nb_tokens++].code = PARSE_MINUS;
			c = cmd[++cmd_pos];
		}
		else if (c == '*') {
			tokens[nb_tokens++].code = PARSE_MULT;
			c = cmd[++cmd_pos];
		}
		else if (c == '/') {
			tokens[nb_tokens++].code = PARSE_DIV;
			c = cmd[++cmd_pos];
		}
		else return -cmd_pos;

//		if (c != ' ' && c != '+' && c != '-' && c != '=' && c != 0) return -cmd_pos;

		NEXT_WORD;
	}

	return 1;
}
