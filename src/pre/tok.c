#include "pre/tok.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

enum {
  TYPE_UNKNOWN,
  TYPE_SEPARATOR,
  TYPE_OPERATOR,
  TYPE_CONTAINER,
  TYPE_LETTER,
  TYPE_DIGIT,
};

const int NUM_KEYWORDS = 8;
const char *KEYWORDS[] = {"say", "return", "method", "if", "elif", "else", "while", "class"};

const int NUM_OPERATORS = 23;
const char *OPERATORS[] = {
    "=",  "+",  "-",  "/", "*",   "%",  ">", "<", ">>", "<<", 
    "==", "<=", ">=", "!=", "and", "or", "!", ".", "<-", "<|", 
    "|", "&", ":"};

const int NUM_UNARY_OPERATORS = 1;
const char *UNARY_OPERATORS[] = {"!"};

const int NUM_BINARY_OPERATORS = 22;
const char *BINARY_OPERATORS[] = {
    "=",  "+",  "-",  "/", "*",   "%",  ">", "<", ">>",
    "<<", "==", "<=", ">=", "!=", "and", "or", ".", "<-", "<|", ":", "|", "&"};

const int NUM_SEPARATORS = 1;
const char *SEPARATORS[] = {","};

const int NUM_CONTAINERS = 6;
const char *CONTAINERS[] = {"(", "{", "[", "]", "}", ")"};

const int NUM_CONTAINER_CLOSERS = 3;
const int NUM_CONTAINER_OPENERS = 3;
const char *CONTAINER_CLOSERS[] = {"}", ")", "]"};
const char *CONTAINER_OPENERS[] = {"{", "(", "["};

int is_letter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int is_digit(char c) { return c >= '0' && c <= '9'; }

int is_operator(string s) {
  for (int i = 0; i < NUM_OPERATORS; ++i) {
    string o = to_string(OPERATORS[i]);
    if (string_eq(s, o)) {
      return TRUE;
    }
  }
  return FALSE;
}

int is_separator(string s) {
  for (size_t i = 0; i < NUM_SEPARATORS; ++i) {
    string o = to_string(SEPARATORS[i]);
    if (string_eq(s, o)) {
      return TRUE;
    }
  }
  return FALSE;
}

int is_container(string s) {
  for (size_t i = 0; i < NUM_CONTAINERS; ++i) {
    string o = to_string(CONTAINERS[i]);
    if (string_eq(s, o)) {
      return TRUE;
    }
  }
  return FALSE;
}

int is_identifier(string str) {
  if (is_keyword(str)) {
    return FALSE;
  }
  
  const char *s = str.value;
  size_t l = str.length;
  if (l < 1)
    return FALSE;
  else if (!is_letter(s[0]) && s[0] != '_')
    return FALSE;

  for (; --l > 1;) {
    if (!is_letter(s[l]) && !is_digit(s[l]) && s[l] != '_') {
      return FALSE;
    }
  }
  return TRUE;
}

int is_keyword(string s) {
  for (size_t i = 0; i < NUM_KEYWORDS; ++i) {
    string o = to_string(KEYWORDS[i]);
    if (string_eq(s, o)) {
      return TRUE;
    }
  }
  return FALSE;
}

int is_container_opener(string s) {
  for (size_t i = 0; i < NUM_CONTAINER_OPENERS; ++i) {
    string o = to_string(CONTAINER_OPENERS[i]);
    if (string_eq(s, o)) {
      return TRUE;
    }
  }
  return FALSE;
}

int is_container_closer(string s) {
  for (size_t i = 0; i < NUM_CONTAINER_CLOSERS; ++i) {
    string o = to_string(CONTAINER_CLOSERS[i]);
    if (string_eq(s, o)) {
      return TRUE;
    }
  }
  return FALSE;
}

string container_opener_to_closer(string o) {
  string c;
  for (size_t i = 0; i < NUM_CONTAINER_OPENERS; ++i) {
    string op = to_string(CONTAINER_OPENERS[i]);
    if (string_eq(o, op)) {
      c = to_string(CONTAINER_CLOSERS[i]);
      break;
    }
  }
  return c;
}


int str_in_arr(const char *arr[], size_t l, string s) {
  for (;l > 0;) {
    string t = to_string(arr[--l]);
    if (string_eq(t, s)) {
      return TRUE;
    }
  }
  return FALSE;
}

int is_unary_oeprator(string s) {
  return str_in_arr(UNARY_OPERATORS, NUM_UNARY_OPERATORS, s);
}

int is_binary_operator(string s) {
  return str_in_arr(BINARY_OPERATORS, NUM_BINARY_OPERATORS, s);
}

size_t extract_number(const char *s, size_t l, size_t i, TOKEN_TYPE *tp) {
  size_t j = i + 1;
  char x = '\0';

  // If the first digit is '0' and the second
  // character is,
  //   - x then number is hexadecimal.
  //   - b then number is binary.
  //   - else, number is octal.
  if (s[i] == '0' && j < l) {
    x = s[j];
    if (x == 'x' || x == 'b') {
      j++;
    } else {
      x = 'o';
    }
  }

  char fl = FALSE;

  for (; j < l; ++j) {
    if (x == 'x') {
      if (!is_digit(s[j]) && s[j] < 'A' && s[j] > 'F' && s[j] < 'a' &&
          s[j] > 'f')
        break;
    } else if (x == 'o') {
      if (!is_digit(s[j]) || s[j] == '8' || s[j] == '9')
        break;
    } else if (x == 'b') {
      if (s[j] != '0' && s[j] != '1')
        break;
    } else if (s[j] == '.') {
      if (fl) {
        break;
      }
      fl = TRUE;
    } else if (!is_digit(s[j])) {
      break;
    }
  }

  if (x == 'x' || x == 'o' || x == 'b' && fl) {
    // error(floating point numbers can only be of base 10)
  }

  switch (x) {
  case 'x':
    *tp = TOKEN_TYPE_LITERAL_INT_HEX;
    break;
  case 'o':
    // If the number is only zero then it is not
    // octal.
    if (j == i+1)
      *tp = TOKEN_TYPE_LITERAL_INT_DECIMAL;
    else
      *tp = TOKEN_TYPE_LITERAL_INT_OCTAL;
    break;
  case 'b':
    *tp = TOKEN_TYPE_LITERAL_INT_BINARY;
    break;
  default:
    *tp = TOKEN_TYPE_LITERAL_INT_DECIMAL;
  }

  if (fl) {
    *tp = TOKEN_TYPE_LITERAL_FLOAT_DECIMAL;
  }

  return j;
}

size_t _tokenize(const char *s, size_t l, Token *tokens) {
  size_t c = 0;
  int col = 1, row = 1;

  for (size_t i = 0; i < l;) {
    int t = TYPE_UNKNOWN;
    TOKEN_TYPE tp = TOKEN_TYPE_UNKNOWN;
    size_t j;
    if (s[i] == ' ' || s[i] == '\t') {
      tp = TOKEN_TYPE_SPACE;
    } else if (s[i] == '\n') {
      tp = TOKEN_TYPE_NEWLINE;
      row++;
      col = 1;
    } else if (is_digit(s[i])) {
      t = TYPE_DIGIT;
    } else if (is_letter(s[i])) {
      t = TYPE_LETTER;
    } else {
      string k = string_new(&s[i], 1);
      if (is_operator(k)) {
        t = TYPE_OPERATOR;
        tp = TOKEN_TYPE_OPERATOR;
      } else if (is_separator(k)) {
        t = TYPE_SEPARATOR;
        tp = TOKEN_TYPE_SEPARATOR;
      } else if (is_container(k)) {
        t = TYPE_CONTAINER;
        tp = TOKEN_TYPE_CONTAINER;
      }
    }

    if (s[i] == '#') {
      for (j = i + 1; j < l; ++j) {
        if (s[j] == '\n')
          break;
      }
      i = j;
      continue;
    } else if (s[i] == '"' || s[i] == '\'') {
      for (j = i + 1; j < l; ++j) {
        if (s[j] == '\\') {
          j += j < l - 1 && s[j + 1] == 'x' ? 2 : 1;
        } else if (s[j] == s[i]) {
          break;
        }
      }
      if (j >= l) {
        printf("failed to find end of string starting at row:%d col:%d\n", row,
               col);
        return -1;
      }
      tp = TOKEN_TYPE_LITERAL_STRING;
      j++;
    } else if (t == TYPE_DIGIT) {
      // Hexadecimal numbers start with 0x
      // Octal numbers start with 0
      // Binary numbers start with 0b
      j = extract_number(s, l, i, &tp);
    } else if (t == TYPE_LETTER) {
      for (j = i + 1; j < l; ++j) {
        if (!is_letter(s[j]) && !is_digit(s[j]) && s[i] != '_') {
          break;
        }
      }
    } else if (t == TYPE_OPERATOR || t == TYPE_CONTAINER ||
               t == TYPE_SEPARATOR) {
      char a[3] = {s[i], 0, 0};
      string k = string_new(a, 1);
      for (j = i + 1; j < l && k.length < 3; ++j) {
        a[k.length++] = s[j];
        if (t == TYPE_OPERATOR && !is_operator(k) ||
            t == TYPE_CONTAINER && !is_container(k) ||
            t == TYPE_SEPARATOR && !is_separator(k)) {
          break;
        }
      }
    } else {
      j = i + 1;
    }

    j = j > l ? l : j;

    if (tokens != NULL) {
      tokens[c].value.value = &s[i];
      tokens[c].value.length = j - i;
      tokens[c].start = i;
      tokens[c].end = j;
      tokens[c].row = row;
      tokens[c].type = tp;
    }

    c++;
    col += j - i - 1;
    i = j;
  }

  if (tokens != NULL) {
    for (size_t m = 0; m < c; ++m) {
      Token token = tokens[m];
      if (token.type != TOKEN_TYPE_UNKNOWN) {
        continue;
      }

      if (string_eqc(token.value, "true") || string_eqc(token.value, "false")) {
        tokens[m].type = TOKEN_TYPE_LITERAL_BOOLEAN;
      } else if (is_keyword(token.value)) {
        tokens[m].type = TOKEN_TYPE_KEYWORD;
      } else if (is_identifier(token.value)) {
        tokens[m].type = TOKEN_TYPE_IDENTIFIER;
      }
    }
  }

  return c;
}

Token *tokenize(const char *s, size_t l, size_t *c) {
  // Since the number of tokens is unknown, the best
  // way is to count the number of tokens that are present
  // in the given string. Now, use the count to allocate
  // space for the tokens.
  int t = _tokenize(s, l, NULL);
  if (t == -1) {
    return NULL;
  }
  *c = t;

  Token *tokens = malloc(sizeof(Token) * (*c));
  _tokenize(s, l, tokens);
  return tokens;
}

void token_info(Token t) {
  printf("value: ");
  for (size_t i = 0; i < t.value.length; ++i) {
    printf("%c", t.value.value[i]);
  }
  printf(" length:%ld start:%ld end:%ld row:%d col:%d type: ", t.value.length,
         t.start, t.end, t.row, t.col);
  switch (t.type) {
  case TOKEN_TYPE_OPERATOR:
    printf("operator");
    break;
  case TOKEN_TYPE_SEPARATOR:
    printf("separator");
    break;
  case TOKEN_TYPE_CONTAINER:
    printf("container");
    break;
  case TOKEN_TYPE_IDENTIFIER:
    printf("identifier");
    break;
  case TOKEN_TYPE_LITERAL_INT_DECIMAL:
    printf("literal.int.decimal");
    break;
  case TOKEN_TYPE_LITERAL_INT_BINARY:
    printf("literal.int.bin");
    break;
  case TOKEN_TYPE_LITERAL_INT_OCTAL:
    printf("literal.int.octal");
    break;
  case TOKEN_TYPE_LITERAL_INT_HEX:
    printf("literal.int.hex");
    break;
  case TOKEN_TYPE_LITERAL_FLOAT_DECIMAL:
    printf("literal.int.float");
    break;
  case TOKEN_TYPE_LITERAL_STRING:
    printf("literal.string");
    break;
  case TOKEN_TYPE_LITERAL_BOOLEAN:
    printf("literal.bool");
    break;
  case TOKEN_TYPE_UNKNOWN:
    printf("unknown");
    break;
  case TOKEN_TYPE_NEWLINE:
    printf("newline");
    break;
  case TOKEN_TYPE_SPACE:
    printf("space");
    break;
  case TOKEN_TYPE_KEYWORD:
    printf("keyword");
    break;
  }
  printf("\n");
}