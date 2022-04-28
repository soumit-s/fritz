#include "runtime/const_pool.h"
#include "runtime/obj.h"

void constant_pool_creator_init(ConstantPoolCreator *c) {
	c->c_lists = 0;
	c->n_lists = 0;
	c->lists = NULL;
}

CONSTANT_ID constant_pool_creator_add_constant(ConstantPoolCreator *p, Constant c) {
	ConstantPoolCreatorList l;
	if (p->n_lists != 0)
		l = p->lists[p->n_lists-1];

	if (p->n_lists == 0 || l.n_consts == l.c_consts) {
		if (p->n_lists == p->c_lists) {
			size_t c_lists = p->c_lists + 10;
			ConstantPoolCreatorList *l = calloc(c_lists, sizeof(ConstantPoolCreatorList));
			memcpy(l, p->lists, p->n_lists * sizeof(ConstantPoolCreatorList));
			free(p->lists);
			p->lists = l;
			p->c_lists = c_lists;
		}

		l.c_consts = 10;
		l.n_consts = 0;
		l.consts = calloc(l.c_consts, sizeof(Constant));

		p->lists[p->n_lists++] = l;
	}

	ConstantPoolCreatorList *t = &p->lists[p->n_lists-1];
	CONSTANT_ID id = (p->n_lists-1) * 10 + t->n_consts;
	t->consts[t->n_consts++] = c;

	return id;
}

int constant_pool_creator_search(ConstantPoolCreator *p, Constant c, CONSTANT_ID *id) {
	size_t idc = 0;
	for (size_t n_l = 0; n_l < p->n_lists; ++n_l) {
		ConstantPoolCreatorList l = p->lists[n_l];
		for (size_t n_c = 0; n_c < l.n_consts; ++n_c, ++idc) {
			Constant x = l.consts[n_c];
			if (x.type != c.type)
				continue;

			switch (c.type) {
			case CONSTANT_TYPE_BOOLEAN:
				if (c.b_value == x.b_value) {
					*id = idc;
					return TRUE;
				}
				break;
			case CONSTANT_TYPE_INT:
				if (c.i_value == x.i_value) {
					*id = idc;
					return TRUE;
				}
				break;
			case CONSTANT_TYPE_FLOAT:
				if (c.f_value == x.f_value) {
					*id = idc;
					return TRUE;
				}
				break;
			case CONSTANT_TYPE_STRING:
				if (string_eq(c.s_value, x.s_value)) {
					*id = idc;
					return TRUE;
				}
				break;
			}
		}
	}
	return FALSE;
}

void constant_pool_init(ConstantPool *p) {
	p->consts = NULL;
	p->n_consts = 0;
}

void constant_pool_destroy(ConstantPool *p) {
	if (p->consts) {
		free(p->consts);
	}
}

void constant_pool_create(ConstantPool *p, ConstantPoolCreator *c) {
	// Calculate the total number of constants.
	size_t n_consts = 0;
	for (size_t i=0; i < c->n_lists; ++i) {
		n_consts += c->lists[i].n_consts;
	}

	p->n_consts = n_consts;
	p->consts = calloc(n_consts, sizeof(Value));

	for (size_t i=0, k=0 ; i < c->n_lists; ++i) {
		ConstantPoolCreatorList l=c->lists[i];
		for (size_t j=0; j < l.n_consts; ++j, ++k) {
			Constant c = l.consts[j];
			Value v;
			switch (c.type) {
			case CONSTANT_TYPE_INT:
				v.type = VALUE_TYPE_INT;
				v.i_value = c.i_value;
				break;
			case CONSTANT_TYPE_FLOAT:
				v.type = VALUE_TYPE_FLOAT;
				v.f_value = c.f_value;
				break;
			case CONSTANT_TYPE_BOOLEAN:
				v.type = VALUE_TYPE_BOOLEAN;
				v.b_value = c.b_value;
				break;
			case CONSTANT_TYPE_STRING:
				v.type = VALUE_TYPE_STRING;
				v.s_value = c.s_value;
				break;
			}
			p->consts[k] = v;
		}
	}
}