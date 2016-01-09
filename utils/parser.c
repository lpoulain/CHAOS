#include "libc.h"
#include "parser.h"
#include "process.h"
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
			tokens[nb_tokens].position = token_start;
			tokens[nb_tokens++].value = (char*)atoi_substr(cmd, token_start, cmd_pos-1);

			if (c != ' ' && c != '+' && c != '-' && c != '*' && c != '/' && c != '=' && c != '(' && c != ')' && c != 0) return -cmd_pos;
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
			tokens[nb_tokens].position = token_start;
			tokens[nb_tokens++].value = word;

			if (c != ' ' && c != '+' && c != '-' && c != '*' && c != '/' && c != '=' && c != '(' && c != ')' && c != 0) return -cmd_pos;
		}
		// +
		else if (c == '+') {
			tokens[nb_tokens].code = PARSE_PLUS;
			tokens[nb_tokens++].position = cmd_pos;
			c = cmd[++cmd_pos];
		}
		else if (c == '-') {
			tokens[nb_tokens].code = PARSE_MINUS;
			tokens[nb_tokens++].position = cmd_pos;
			c = cmd[++cmd_pos];
		}
		else if (c == '*') {
			tokens[nb_tokens].code = PARSE_MULT;
			tokens[nb_tokens++].position = cmd_pos;
			c = cmd[++cmd_pos];
		}
		else if (c == '/') {
			tokens[nb_tokens].code = PARSE_DIV;
			tokens[nb_tokens++].position = cmd_pos;
			c = cmd[++cmd_pos];
		}
		else if (c == '(') {
			tokens[nb_tokens].code = PARSE_PARENTHESE_OPEN;
			tokens[nb_tokens++].position = cmd_pos;
			c = cmd[++cmd_pos];
		}
		else if (c == ')') {
			tokens[nb_tokens].code = PARSE_PARENTHESE_CLOSE;
			tokens[nb_tokens++].position = cmd_pos;
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
int is_math_formula(int start, int end, int *value) {
	// Empty equation => NOT a valid formula
	if (start >= end) return 0;

	int val1, val2, idx = start, old_idx, res;

	// We need to make sure * and / (major operations) take precedence over
	// + and - (minor operations), no matter what the order
	// For this we consider a formula to be like:
	// val1 * val2 * ... * valn + valn+1 * ... * valm + ...
	//
	// We thus use two accumulators: one for "major operations groups" (e.g. val1 * val2 * .. * valn)
	// and a minor operations accumulator that sums/substracts all the values from the major op groups
	int accum_minor = 0, accum_major = 1, major_op = PARSE_MULT, minor_op = PARSE_PLUS, op=0;

	while (idx < end) {

		// Get the next "number" (parenthese or otherwise)

		// The beginning is a valid parenthesis
		if (tokens[idx].code == PARSE_PARENTHESE_OPEN) {
			int nb_parenth = 1;
			old_idx = idx;
			idx++;
			while (nb_parenth > 0 && idx < end) {
				if (tokens[idx].code == PARSE_PARENTHESE_OPEN) nb_parenth++;
				if (tokens[idx].code == PARSE_PARENTHESE_CLOSE) nb_parenth--;
				if (nb_parenth < 0) {
					error("Too many )'s");
					return -tokens[idx].position;
				}
				idx++;
			}
			if (nb_parenth > 0) {
				error("Missing a )");
				return -tokens[idx-1].position;
			}

			// (...) is not a valid equation
			res = is_math_formula(old_idx+1, idx-1, value); 
			if (res <= 0) return res;
		}

		// The beginning is a number
		else if (tokens[idx].code == PARSE_NUMBER) {
			*value = (int)tokens[idx].value;
			idx++;
		}

		// wrong character
		else {
			error("Wrong symbol");
			return -tokens[idx].position;
		}

		// We have the value, let's mult/div it to the major accumulator
		res = operation(accum_major, major_op, *value, &accum_major);
		if (!res) return -tokens[idx-1].position;

		// No more argument. We're done
		if (idx == end) {
			// Let's add accum_major to the minor accumulator
			res = operation(accum_minor, minor_op, accum_major, &accum_minor);
			if (!res) return -tokens[idx-1].position;

			// and we have the result
			*value = accum_minor;
			return 1;
		}

		// Check the next symbol is an operation
		if (idx + 1 > end || (tokens[idx].code != PARSE_PLUS &&
							  tokens[idx].code != PARSE_MINUS &&
							  tokens[idx].code != PARSE_MULT &&
							  tokens[idx].code != PARSE_DIV) ) {
			error("Expected an operation");
			return -tokens[idx].position;
		}

		op = tokens[idx].code;

		// We end the series of major operations
		if (op == PARSE_PLUS || op == PARSE_MINUS) {
			res = operation(accum_minor, minor_op, accum_major, &accum_minor);
			if (!res) return -tokens[idx-1].position;
			
			accum_major = 1;
			major_op = PARSE_MULT;
			minor_op = op;
		}
		else major_op = op;

		idx++;
	}

	error("Formula incomplete");
	return -end;
}
