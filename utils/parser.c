#include "libc.h"
#include "parser.h"
#include "process.h"
#include "heap.h"

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

Token **parser_new_token(Token **tokens, uint code, uint position, char *value) {
	(*tokens) = (Token*)malloc(sizeof(Token));
	(*tokens)->code = code;
	(*tokens)->position = position;
	(*tokens)->value = value;
	(*tokens)->next = 0;

	return &((*tokens)->next);
}

uint parse(char *cmd, Token **tokens) {
	int parse_state = PARSE_UNDEFINED;
	int cmd_pos = 0, token_start, token_end;

	char c = cmd[cmd_pos];
	char *word;
	Token *token = *tokens;

	NEXT_WORD;

	while (c != 0) {
		// Number
		if (c >= '0' && c <= '9') {
			token_start = cmd_pos;
			c = cmd[++cmd_pos];
			while (c >= '0' && c <= '9' && c != 0) c = cmd[++cmd_pos];
			tokens = parser_new_token(tokens, PARSE_NUMBER, token_start, (char*)atoi_substr(cmd, token_start, cmd_pos-1));

			if (c != ' ' && c != '+' && c != '-' && c != '*' && c != '/' && c != '=' && c != '(' && c != ')' && c != 0) return -cmd_pos;
		}
		// Symbol
		else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
			token_start = cmd_pos;
			c = cmd[++cmd_pos];
			while (((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_')) && c != 0) c = cmd[++cmd_pos];

			word = (char *)malloc(cmd_pos - token_start + 1);
			strncpy(word, cmd + token_start, cmd_pos - token_start);
			word[cmd_pos - token_start] = 0;

			tokens = parser_new_token(tokens, PARSE_WORD, token_start, word);

			if (c != ' ' && c != '+' && c != '-' && c != '*' && c != '/' && c != '=' && c != '(' && c != ')' && c != 0) return -cmd_pos;
		}
		// +
		else if (c == '+') {
			tokens = parser_new_token(tokens, PARSE_PLUS, cmd_pos, 0);
			c = cmd[++cmd_pos];
		}
		else if (c == '-') {
			tokens = parser_new_token(tokens, PARSE_MINUS, cmd_pos, 0);
			c = cmd[++cmd_pos];
		}
		else if (c == '*') {
			tokens = parser_new_token(tokens, PARSE_MULT, cmd_pos, 0);
			c = cmd[++cmd_pos];
		}
		else if (c == '/') {
			tokens = parser_new_token(tokens, PARSE_DIV, cmd_pos, 0);
			c = cmd[++cmd_pos];
		}
		else if (c == '(') {
			tokens = parser_new_token(tokens, PARSE_PARENTHESE_OPEN, cmd_pos, 0);
			c = cmd[++cmd_pos];
		}
		else if (c == ')') {
			tokens = parser_new_token(tokens, PARSE_PARENTHESE_CLOSE, cmd_pos, 0);
			c = cmd[++cmd_pos];
		}
		else return -cmd_pos;

//		if (c != ' ' && c != '+' && c != '-' && c != '=' && c != 0) return -cmd_pos;

		NEXT_WORD;
	}

	return 1;
}

int operation(int val1, int op, int val2, int *result) {
	switch(op) {
		case PARSE_PLUS:
			*result = val1 + val2;
			return 1;
		case PARSE_MINUS:
			*result = val1 - val2;
			return 1;
		case PARSE_MULT:
			*result = val1 * val2;
			return 1;
		case PARSE_DIV:
			if (val2 == 0) {
				error("Division by zero");
				return 0;
			}
			*result = val1 / val2;
			return 1;
	}

	error("Unknown operation");
	return 0;
}

// Returns a positive number if the formula is valid
// 0 => not a valid formula
// Negative number: the character where the error is found
int is_math_formula(Token *start, Token *end, int *value) {
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
			res = is_math_formula(old_token1->next, old_token2, value); 
			if (res <= 0) {
				error("Not a math formula");
				return res;
			}
		}

		// The beginning is a number
		else if (token->code == PARSE_NUMBER) {
			*value = (int)token->value;
			old_token1 = token;
			token = token->next;
		}

		// wrong character
		else {
			error("Wrong symbol");
			return -token->position;
		}

		// We have the value, let's mult/div it to the major accumulator
		res = operation(accum_major, major_op, *value, &accum_major);
		if (!res) return -old_token1->position;

		// No more argument. We're done
		if (token == end) {
			// Let's add accum_major to the minor accumulator
			res = operation(accum_minor, minor_op, accum_major, &accum_minor);
			if (!res) return -old_token1->position;

			// and we have the result
			*value = accum_minor;
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
			res = operation(accum_minor, minor_op, accum_major, &accum_minor);
			if (!res) return -token->position;
			
			accum_major = 1;
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

void parser_print_tokens(Token *tokens) {
	while (tokens) {
		switch(tokens->code) {
			case PARSE_WORD:
				printf("[%s] ", tokens->value);
				break;
			case PARSE_NUMBER:
				printf("[%d] ", tokens->value);
				break;
			case PARSE_PLUS:
				printf("[+] ");
				break;
			case PARSE_MINUS:
				printf("[-] ");
				break;
			case PARSE_MULT:
				printf("[*] ");
				break;
			case PARSE_DIV:
				printf("[/] ");
				break;
			case PARSE_PARENTHESE_OPEN:
				printf("[(] ");
				break;
			case PARSE_PARENTHESE_CLOSE:
				printf("[)] ");
				break;
			default:
				printf("[?] ");
		}
		tokens = tokens->next;
	}
	printf("\n");
}

void parser_memory_cleanup(Token *tokens) {
	while (tokens) {
		if (tokens->code == PARSE_WORD) free(tokens->value);
		Token *tmp = tokens;
		tokens = tokens->next;
		free(tmp);
	}
}
