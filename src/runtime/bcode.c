#include "runtime/bcode.h"
#include "runtime/const_pool.h"
#include "runtime/utils.h"

#include <string.h>
#include <stdio.h>

void bcode_buffer_init(BcodeBuffer *b) {
	b->end.prev = NULL;
}

void bcode_buffer_destroy(BcodeBuffer *b) {
	for (BcodeBufferIterator *i = b->end.prev; i != NULL; ) {
		BcodeBufferIterator *t = i->prev;
		free(i->fragment.ptr);
		free(i);
		i = t;
	}
}

BcodeBufferIterator* bcode_buffer_end(BcodeBuffer *b) {
	return b->end.prev;
}

void bcode_buffer_append(BcodeBuffer *b, const uint8_t *ptr, size_t l) {
	uint8_t *f = calloc(l, sizeof(uint8_t));
	memcpy(f, ptr, l);

	BcodeBufferIterator *i = calloc(1, sizeof(BcodeBufferIterator));
	i->fragment.ptr = f;
	i->fragment.length = l;

	i->prev = b->end.prev;
	b->end.prev = i;
}

size_t bcode_buffer_size(BcodeBuffer* b) {
	size_t s=0;
	for(BcodeBufferIterator *i = bcode_buffer_end(b); i != NULL; i=i->prev) {
		s += i->fragment.length;
	}
	return s;
}

BcodeBufferLabel bcode_buffer_pin_label(BcodeBuffer *b) {
	BcodeBufferIterator *end = bcode_buffer_end(b);
	BcodeBufferLabel l;
	l.iter = end;
	l.pos = end->fragment.length;
	return l;
}

size_t bcode_buffer_label_calculate_offset(BcodeBufferLabel l, BcodeBufferLabel u) {
	BcodeBufferIterator *iter = u.iter;
	size_t offset = u.pos;

	if (u.iter == l.iter) {
		return u.pos - l.pos;
	}

	for (iter=iter->prev ;iter != l.iter && iter != NULL; iter = iter->prev) {
		offset += iter->fragment.length;
	}

	if (iter != NULL) {
		offset += iter->fragment.length - l.pos;
	}

	return offset;
}

void bcode_buffer_to_bcode(BcodeBuffer *b_buff, Bcode *b) {
	size_t length=bcode_buffer_size(b_buff);
	uint8_t* buffer = calloc(length, sizeof(uint8_t));

	size_t j=length;
	for (BcodeBufferIterator *i=bcode_buffer_end(b_buff); i != NULL; i = i->prev) {
		BcodeBufferFragment f = i->fragment;

		j -= f.length;
		memcpy(&buffer[j], f.ptr, f.length);
	}

	b->buffer = buffer;
	b->length = length;
}

#define OPCODE_EQUALS(x, y) (x)[0]==y[0] && (x)[1]==y[1]

const char* _opcode_to_inst(const uint8_t* opcode) {
	#define X(o, n) if (OPCODE_EQUALS(opcode, OPCODE_##o)) { return n; }
		INST_TO_OPCODE_MAP
	#undef X
	return NULL;
}

const char* bcode_opcode_to_inst(const uint8_t* opcode) {
	return _opcode_to_inst(opcode);
}

void bcode_dump(Bcode *bc, ConstantPool *p) {
	const uint8_t* b = bc->buffer;
	size_t l = bc->length;

	for (size_t i=0; i < l-1; ) {
		const uint8_t *addr = &b[i];

		if (OPCODE_EQUALS(addr, OPCODE_ADD) ||
			OPCODE_EQUALS(addr, OPCODE_SUB) ||
			OPCODE_EQUALS(addr, OPCODE_MUL) ||
			OPCODE_EQUALS(addr, OPCODE_DIV) ||
			OPCODE_EQUALS(addr, OPCODE_MOD) ||
			OPCODE_EQUALS(addr, OPCODE_GT) || 
			OPCODE_EQUALS(addr, OPCODE_LT) ||
			OPCODE_EQUALS(addr, OPCODE_GTE) ||
			OPCODE_EQUALS(addr, OPCODE_LTE) || 
			OPCODE_EQUALS(addr, OPCODE_EQ) ||
			OPCODE_EQUALS(addr, OPCODE_NEQ) ||
			OPCODE_EQUALS(addr, OPCODE_SET_OBJECT) ||
			OPCODE_EQUALS(addr, OPCODE_GET_NULL) ||
			OPCODE_EQUALS(addr, OPCODE_METHOD_CREATE) ||
			OPCODE_EQUALS(addr, OPCODE_OBJECT_CREATE) ||
			OPCODE_EQUALS(addr, OPCODE_BLOCK_EXPLICIT_END) ||
			OPCODE_EQUALS(addr, OPCODE_BLOCK_IMPLICIT_END) ||
			OPCODE_EQUALS(addr, OPCODE_BLOCK_NOEXEC_END) ||
			OPCODE_EQUALS(addr, OPCODE_RETURN) ||
			OPCODE_EQUALS(addr, OPCODE_RETURN_NULL) ||
			OPCODE_EQUALS(addr, OPCODE_GET_OBJECT) ||
			OPCODE_EQUALS(addr, OPCODE_POP) ||
			OPCODE_EQUALS(addr, OPCODE_STACK_DUP_TOP) ||
			OPCODE_EQUALS(addr, OPCODE_CLASSIFY) ||
			OPCODE_EQUALS(addr, OPCODE_SET_SCOPE_STACK)
			) {
			printf("%s\n", _opcode_to_inst(addr));
			i += OPCODE_SIZE;
		} else if (
				OPCODE_EQUALS(addr, OPCODE_GET_CONSTANT) ||
				OPCODE_EQUALS(addr, OPCODE_GET_SCOPE) ||
				OPCODE_EQUALS(addr, OPCODE_SET_SCOPE) ||
				OPCODE_EQUALS(addr, OPCODE_SET_SCOPE_NULL)
			) {
			printf("%s ", _opcode_to_inst(addr));
			CONSTANT_ID id = *(const CONSTANT_ID*)(addr + 2);
			printf("%ld\n", id);
			i += sizeof(CONSTANT_ID) + OPCODE_SIZE;
		} else if (
			OPCODE_EQUALS(addr, OPCODE_BLOCK_NOEXEC_START) ||
			OPCODE_EQUALS(addr, OPCODE_BLOCK_IMPLICIT_START) ||
			OPCODE_EQUALS(addr, OPCODE_BLOCK_EXPLICIT_START) ||
			OPCODE_EQUALS(addr, OPCODE_JUMP) ||
			OPCODE_EQUALS(addr, OPCODE_JUMP_IF) ||
			OPCODE_EQUALS(addr, OPCODE_JUMP_NIF) ||
			OPCODE_EQUALS(addr, OPCODE_JUMP_IF_NOPOP) ||
			OPCODE_EQUALS(addr, OPCODE_JUMP_NIF_NOPOP) ||
			OPCODE_EQUALS(addr, OPCODE_JUMP_BACK)
			) {
			printf("%s ", _opcode_to_inst(addr));
			size_t n = *(const size_t*)(addr + 2);
			printf("%ld\n", n);
			i += OPCODE_SIZE + sizeof(size_t);
		} else if (OPCODE_EQUALS(addr, OPCODE_INVOKE_CONSTANT) ||
				OPCODE_EQUALS(addr, OPCODE_INVOKE_CONSTANT_ASYNC)) {
			printf("%s ", _opcode_to_inst(addr));
			FzInt num_params = *(const FzInt*)(addr + 2);
			printf("%ld\n", num_params);
			i += OPCODE_SIZE + sizeof(FzInt);
		} else {
			printf("unknown opcode (0x%x, 0x%x)\n", addr[0], addr[1]);
			return;
		}
	}
}