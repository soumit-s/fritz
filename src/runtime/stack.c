#include "runtime/stack.h"

void stack_init(Stack *s) {
	s->capacity = 0;
	s->values = NULL;
	s->top_ptr = NULL;
}

void stack_create(Stack *s, size_t n) {
	s->values = calloc(n, sizeof(Value));
	s->top_ptr = s->values;
	s->capacity = n;
}

void stack_destroy(Stack *s) {
	if (s->values != NULL) {
		free(s->values);
	}
}