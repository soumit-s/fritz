#include "str.h"
#include <string.h>

string string_new(const char *v, size_t l) {
  string s = {v, l};
  return s;
}

string to_string(const char *v) {
  string s = {v, (size_t)strlen(v)};
  return s;
}

int string_eq(string s1, string s2) {
  if (s1.length != s2.length) {
    return FALSE;
  }
  for (size_t i = s1.length; i > 0; --i) {
    if (s1.value[i - 1] != s2.value[i - 1]) {
      return FALSE;
    }
  }
  return TRUE;
}

int string_eqc(string s, const char *c) { return string_eq(s, to_string(c)); }

long string_to_long(string s) {
  long n = 0;
  if (s.length == 0)
    return n;

  int msb = 1;
  size_t i = 0;
  if (s.value[0] == '-') {
    i++;
    msb = -1;
  }

  for (; i < s.length; ++i) {
    n = n * 10 + (s.value[i] - '0');
  }

  return msb * n;
}

long string_hex_to_long(string s) {
  long n = 0;
  if (s.length == 0)
    return n;

  int msb = 1;
  size_t i = 0;
  if (s.value[0] == '-') {
    i++;
    msb = -1;
  }

  for (int j = s.length - 1, f = 1; i <= j; --j, f *= 16) {
    char ch = s.value[j];
    long d = 0;
    if (ch >= '0' && ch <= '9') {
      d = ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
      d = ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'F') {
      d = ch - 'A' + 10;
    }
    n += f * d;
  }

  return msb * n;
}

long string_octal_to_long(string s) {
  long n = 0;
  if (s.length == 0)
    return n;

  int msb = 1;
  size_t i = 0;
  if (s.value[0] == '-') {
    i++;
    msb = -1;
  }

  for (int j = s.length - 1, f = 1; i <= j; --j, f *= 8) {
    char ch = s.value[j];
    long d = 0;
    if (ch >= '0' && ch <= '7') {
      d = ch - '0';
    }
    n += f * d;
  }

  return msb * n;
}

long string_bin_to_long(string s) {
  long n = 0;
  if (s.length == 0)
    return n;

  int msb = 1;
  size_t i = 0;
  if (s.value[0] == '-') {
    i++;
    msb = -1;
  }

  for (int j = s.length - 1, f = 1; i <= j; --j, f *= 2) {
    char ch = s.value[j];
    long d = 0;
    if (ch >= '0' && ch <= '1') {
      d = ch - '0';
    }
    n += f * d;
  }

  return msb * n;
}

double string_to_double(string s) {
  double n = 0;
  if (s.length == 0)
    return n;

  int msb = 1;
  size_t i = 0;
  if (s.value[0] == '-') {
    i++;
    msb = -1;
  }

  for (;i < s.length && s.value[i] != '.'; ++i) {
  	n = n * 10 + (s.value[i] - '0');
  }

  ++i;
  for (double f=0.1; i < s.length; ++i, f*=0.1) {
  	n += (s.value[i] - '0') * f;
  }

  return msb * n;
}

string string_to_string(string s) {
  string ns;

  size_t c=0;
  for (size_t i=0; i < s.length; ++i, ++c) {
    if (s.value[i] == '\\' && i < s.length-1) {
      switch (s.value[i+1]) {
      case 'n':
      case 't':
      case '\'':
      case '\"':
        i+=1;
        break;
      }
    } 
  }

  char *str = calloc(c, sizeof(char));

  ns.value = str;
  ns.length = c;

  c = 0;
  for (size_t i=0; i < s.length; ++i, ++c) {
    if (s.value[i] == '\\' && i < s.length-1) {
      char ch = s.value[i+1];
      if (ch == 'n') {
        str[c] = '\n';
        i++; 
      } else if (ch == 't') {
        str[c] = '\t';
        i++;
      } else if (ch == '\"') {
        str[c] = '\"';
        i++;
      } else if (ch == '\'') {
        str[c] = '\'';
        i++;
      }
    } else {
      str[c] = s.value[i];
    }
  }

  return ns;
}
