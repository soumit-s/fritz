#pragma once
#include <stdlib.h>
#include "str.h"

extern const int NUM_KEYWORDS;
extern const int NUM_OPERATORS;
extern const int NUM_UNARY_OPERATORS;
extern const int NUM_BINARY_OPERATORS;
extern const int NUM_SEPARATORS;
extern const int NUM_CONTAINERS;
extern const int NUM_CONTAINER_CLOSERS;
extern const int NUM_CONTAINER_OPENERS;

typedef enum {
	TOKEN_TYPE_UNKNOWN,
	TOKEN_TYPE_IDENTIFIER,
	TOKEN_TYPE_KEYWORD,
	TOKEN_TYPE_OPERATOR,
	TOKEN_TYPE_SEPARATOR,
	TOKEN_TYPE_CONTAINER,
	TOKEN_TYPE_LITERAL_STRING,
	TOKEN_TYPE_LITERAL_INT_DECIMAL,
	TOKEN_TYPE_LITERAL_INT_HEX,
	TOKEN_TYPE_LITERAL_INT_OCTAL,
	TOKEN_TYPE_LITERAL_INT_BINARY,
	TOKEN_TYPE_LITERAL_FLOAT_DECIMAL,
	TOKEN_TYPE_LITERAL_BOOLEAN,
	TOKEN_TYPE_SPACE,
	TOKEN_TYPE_NEWLINE,
} TOKEN_TYPE;

extern const char* OPERATORS[]; 
extern const char* UNARY_OPERATORS[];
extern const char* BINARY_OPERATORS[];

extern const char* SEPARATORS[]; 

extern const char* CONTAINERS[]; 

extern const char* KEYWORDS[];

extern const char* CONTAINER_CLOSERS[];
extern const char* CONTAINER_OPENERS[];

typedef struct fz_token Token;

struct fz_token {
	string value;
	size_t start;
	size_t end;
	int col;
	int row;
	TOKEN_TYPE type;
};

extern int is_letter(char);
extern int is_digit(char);

extern int is_operator(string s);
extern int is_separator(string s);
extern int is_container(string s);

extern int is_identifier(string);
extern int is_keyword(string);

extern int is_container_opener(string);
extern int is_container_closer(string);

extern string container_opener_to_closer(string);

extern int is_unary_oeprator(string);
extern int is_binary_operator(string);

extern Token* tokenize(const char*, size_t, size_t*);
extern void token_info(Token);