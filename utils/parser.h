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

typedef struct {
	uint code;
	void *value;
} token;

uint parse(char *cmd);

#endif
