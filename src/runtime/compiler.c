#include "runtime/compiler.h"
#include "pre/ast.h"
#include "pre/tok.h"
#include "runtime/bcode.h"

#define __OPCODE_MAPPER(x, y)                                                  \
  if (string_eqc(s, x))                                                        \
  return y

const uint8_t *_map_opcode(string s) {
  __OPCODE_MAPPER("+", OPCODE_ADD);
  __OPCODE_MAPPER("-", OPCODE_SUB);
  __OPCODE_MAPPER("*", OPCODE_MUL);
  __OPCODE_MAPPER("/", OPCODE_DIV);
  __OPCODE_MAPPER("%", OPCODE_MOD);
  return NULL;
}

void instance_compile_keyword_node(Instance *i, AstNode n, BcodeBuffer *b) {
  Token v = n.value;
  if (string_eqc(v.value, "let")) {
  }
}

void instance_compile_literal(Instance *i, AstNode n, BcodeBuffer *b,
                              ConstantPoolCreator *p) {
  Token t = n.value;
  Constant c;
  switch (t.type) {
  case TOKEN_TYPE_LITERAL_INT_DECIMAL:
    c.i_value = string_to_long(t.value);
    c.type = CONSTANT_TYPE_INT;
    break;
  case TOKEN_TYPE_LITERAL_INT_BINARY:
    c.i_value = string_bin_to_long(t.value);
    c.type = CONSTANT_TYPE_INT;
    break;
  case TOKEN_TYPE_LITERAL_INT_OCTAL:
    c.i_value = string_octal_to_long(t.value);
    c.type = CONSTANT_TYPE_INT;
    break;
  case TOKEN_TYPE_LITERAL_INT_HEX:
    c.i_value = string_hex_to_long(t.value);
    c.type = CONSTANT_TYPE_INT;
    break;

  case TOKEN_TYPE_LITERAL_FLOAT_DECIMAL:
    // Convert to 8 byte floating point number.
    c.f_value = string_to_double(t.value);
    c.type = CONSTANT_TYPE_FLOAT;

  case TOKEN_TYPE_LITERAL_STRING:
    // Decrease the length by two to eliminate the opening
    // and closing quotes.
    c.s_value.length = t.value.length - 2;

    // Skipping the opening quote of the string.
    c.s_value.value = t.value.value + 1;

    c.type = CONSTANT_TYPE_STRING;
  case TOKEN_TYPE_LITERAL_BOOLEAN:
    c.b_value = string_eqc(t.value, "true");
    c.type = CONSTANT_TYPE_BOOLEAN;
    break;
  default:
    break;
  }

  // Add it to the constant pool creator and retrieve the
  // constant id.
  CONSTANT_ID id;
  if (!constant_pool_creator_search(p, c, &id)) {
    id = constant_pool_creator_add_constant(p, c);
  }

  bcode_buffer_append(b, OPCODE_GET_CONSTANT, OPCODE_SIZE);
  bcode_buffer_append(b, (const uint8_t *)&id, sizeof(id));
}

void instance_compile_pattern_matcher(Instance *i, AstNode n, BcodeBuffer *b,
                                      ConstantPoolCreator *p) {
  AstNode pattern = n.children[0];

  if (pattern.type == NODE_TYPE_CONTAINER) {
    if (string_eqc(pattern.value.value, "{")) {

    } else if (string_eqc(pattern.value.value, "(")) {

    } else if (string_eqc(pattern.value.value, "[")) {
    }
  } else if (pattern.type == NODE_TYPE_TOKEN) {
    instance_compile_node(i, n.children[1], b, p);

    Constant c;
    c.type = CONSTANT_TYPE_STRING;
    c.s_value = pattern.value.value;
    CONSTANT_ID id;
    if (!constant_pool_creator_search(p, c, &id)) {
      id = constant_pool_creator_add_constant(p, c);
    }

    bcode_buffer_append(b, OPCODE_SET_SCOPE, OPCODE_SIZE);
    bcode_buffer_append(b, (const uint8_t*)&id, sizeof(CONSTANT_ID));
  }
}

void instance_compile_identifier(Instance *i, AstNode n, BcodeBuffer *b,
                                 ConstantPoolCreator *p) {
  Constant c;
  c.type = CONSTANT_TYPE_STRING;
  c.s_value = n.value.value;
  CONSTANT_ID id;
  if (!constant_pool_creator_search(p, c, &id)) {
    id = constant_pool_creator_add_constant(p, c);
  }

  bcode_buffer_append(b, OPCODE_GET_SCOPE, OPCODE_SIZE);
  bcode_buffer_append(b, (const uint8_t *)&id, sizeof(CONSTANT_ID));
}


void instance_compile_cont_call(Instance *i, AstNode n, BcodeBuffer *b, ConstantPoolCreator *p) {
  // hello (a, b)
  // Here 'hello' is the source node (src)
  // Here (a, b) is the container node (cont)
  AstNode src_n = n.children[0];
  AstNode cont_n = n.children[1];

  // If container node is '(', then the cont call
  // is a function call.
  // If container node is '[', then the cont call
  // is an array access operation.

  if (string_eqc(cont_n.value.value, "(")) {
    // Compile the node that will put the function to be called on the stack.
    instance_compile_node(i, src_n, b, p);
    // Compile the parameters and push them onto the stack.
    FzInt param_count = 0;

    if (cont_n.children) {
      for (size_t j=0; j < cont_n.n_children; ++j) {
        AstNode c = cont_n.children[j];
        if (c.type == NODE_TYPE_SEPARATOR) {
          for (size_t k=0; k < c.n_children; ++k, ++param_count)
            instance_compile_node(i, c.children[k], b, p);
        } else {
          instance_compile_node(i, c, b, p);
          param_count++;
        }
      }
    }
    bcode_buffer_append(b, OPCODE_INVOKE_CONSTANT, OPCODE_SIZE);
    bcode_buffer_append(b, (const uint8_t*)&param_count, sizeof(FzInt));
  } 
}

void instance_compile_node(Instance *i, AstNode n, BcodeBuffer *b,
                           ConstantPoolCreator *p) {
  Token v = n.value;
  if (n.type == NODE_TYPE_BINARY_OPERATOR) {
    if (string_eqc(v.value, "=")) {

      instance_compile_pattern_matcher(i, n, b, p);

    } else if (string_eqc(v.value, "<-")) {

    } else {
      instance_compile_node(i, n.children[0], b, p);
      instance_compile_node(i, n.children[1], b, p);

      const uint8_t *opcode = _map_opcode(v.value);
      bcode_buffer_append(b, opcode, 2);
    }
  } else if (n.type == NODE_TYPE_UNARY_OPERATOR) {

    instance_compile_node(i, n.children[0], b, p);
    const uint8_t *opcode = _map_opcode(v.value);
    bcode_buffer_append(b, opcode, 2);

  } else if (n.type == NODE_TYPE_CONTAINER) {
    const uint8_t *s_op, *e_op;
    if (string_eqc(v.value, "{")) {
      s_op = OPCODE_BLOCK_EXPLICIT_START;
      e_op = OPCODE_BLOCK_EXPLICIT_END;
    } /*else if (string_eqc(v.value, "(")) {
            s_op = OPCODE_BLOCK_IMPLICIT_START;
            e_op = OPCODE_BLOCK_IMPLICIT_END;
    }*/

    // bcode_buffer_append(b, s_op, 2);

    for (size_t j = 0; j < n.n_children; ++j) {
      instance_compile_node(i, n.children[j], b, p);
    }

    // bcode_buffer_append(b, e_op, 2);
  } else if (n.type == NODE_TYPE_KEYWORD) {

    instance_compile_keyword_node(i, n, b);

  } else if (n.type == NODE_TYPE_TOKEN) {
    switch (v.type) {
    case TOKEN_TYPE_LITERAL_INT_DECIMAL:
    case TOKEN_TYPE_LITERAL_INT_BINARY:
    case TOKEN_TYPE_LITERAL_INT_OCTAL:
    case TOKEN_TYPE_LITERAL_INT_HEX:
    case TOKEN_TYPE_LITERAL_FLOAT_DECIMAL:
    case TOKEN_TYPE_LITERAL_BOOLEAN:
    case TOKEN_TYPE_LITERAL_STRING:
      instance_compile_literal(i, n, b, p);
      break;
    case TOKEN_TYPE_IDENTIFIER:
      instance_compile_identifier(i, n, b, p);
      break;
    default:
      break;
    }
  } else if (n.type == NODE_TYPE_CONT_CALL) {
    instance_compile_cont_call(i, n, b, p);
  }
}
