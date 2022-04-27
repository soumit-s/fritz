#include "pre/parser.h"

#include "str.h"
#include <string.h>

const char *_precedence_list_0[] = {"."}, *_precedence_list_1[] = {"!"},
           *_precedence_list_2[] = {"/", "*", "%"},
           *_precedence_list_3[] = {"+", "-"},
           *_precedence_list_4[] = {">>", "<<"},
           *_precedence_list_5[] = {"&", "|"},
           *_precedence_list_6[] = {">", "<", ">=", "<="},
           *_precedence_list_7[] = {"==", "!="},
           *_precedence_list_8[] = {"and", "or"},
           *_precedence_list_9[] = {"<-", "<|"},
           *_precedence_list_10[] = {"=>"},
           *_precedence_list_11[] = {"="};

const PRECEDENCE PRECEDENCE_LIST[] = {
    {1, ASSOCIATIVITY_LEFT_TO_RIGHT, _precedence_list_0},
    {1, ASSOCIATIVITY_LEFT_TO_RIGHT, _precedence_list_1},
    {3, ASSOCIATIVITY_LEFT_TO_RIGHT, _precedence_list_2},
    {2, ASSOCIATIVITY_LEFT_TO_RIGHT, _precedence_list_3},
    {2, ASSOCIATIVITY_LEFT_TO_RIGHT, _precedence_list_4},
    {2, ASSOCIATIVITY_LEFT_TO_RIGHT, _precedence_list_5},
    {4, ASSOCIATIVITY_LEFT_TO_RIGHT, _precedence_list_6},
    {2, ASSOCIATIVITY_LEFT_TO_RIGHT, _precedence_list_7},
    {2, ASSOCIATIVITY_LEFT_TO_RIGHT, _precedence_list_8},
    {2, ASSOCIATIVITY_LEFT_TO_RIGHT, _precedence_list_9},
    {1, ASSOCIATIVITY_LEFT_TO_RIGHT, _precedence_list_10},
    {1, ASSOCIATIVITY_RIGHT_TO_LEFT, _precedence_list_11},
};

const int PRECEDENCE_LIST_LENGTH = 12;