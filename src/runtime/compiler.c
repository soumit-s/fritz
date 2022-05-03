#include "runtime/compiler.h"
#include "pre/ast.h"
#include "pre/meta.h"
#include "pre/tok.h"
#include "runtime/bcode.h"
#include "runtime/const_pool.h"

#define __OPCODE_MAPPER(x, y)                                                  \
  if (string_eqc(s, x))                                                        \
  return y

const uint8_t *_map_opcode(string s) {
  __OPCODE_MAPPER("+", OPCODE_ADD);
  __OPCODE_MAPPER("-", OPCODE_SUB);
  __OPCODE_MAPPER("*", OPCODE_MUL);
  __OPCODE_MAPPER("/", OPCODE_DIV);
  __OPCODE_MAPPER("%", OPCODE_MOD);
  __OPCODE_MAPPER("==", OPCODE_EQ);
  __OPCODE_MAPPER("!=", OPCODE_NEQ);
  __OPCODE_MAPPER(">=", OPCODE_GTE);
  __OPCODE_MAPPER("<=", OPCODE_LTE);
  __OPCODE_MAPPER(">", OPCODE_GT);
  __OPCODE_MAPPER("<", OPCODE_LT);
  return NULL;
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
    break;

  case TOKEN_TYPE_LITERAL_STRING: {
    string str;

    // Decrease the length by two to eliminate the opening
    // and closing quotes.
    str.length = t.value.length - 2;

    // Skipping the opening quote of the string.
    str.value = t.value.value + 1;

    // Create a new string with the escape sequences
    // replaced by the characters that they represent.
    c.s_value = string_to_string(str);

    c.type = CONSTANT_TYPE_STRING;
    break;
  }
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

void instance_compile_node_as_literal(Instance *i, AstNode n, BcodeBuffer *b,
                                      ConstantPoolCreator *p) {
  if (n.value.type == TOKEN_TYPE_IDENTIFIER) {
    Constant c;
    c.type = CONSTANT_TYPE_STRING;
    c.s_value = n.value.value;

    CONSTANT_ID id;
    if (!constant_pool_creator_search(p, c, &id)) {
      id = constant_pool_creator_add_constant(p, c);
    }

    bcode_buffer_append(b, OPCODE_GET_CONSTANT, OPCODE_SIZE);
    bcode_buffer_append(b, (const uint8_t *)&id, sizeof(id));
  } else {
    instance_compile_literal(i, n, b, p);
  }
}

void instance_compile_nodes(Instance *i, AstNode *nodes, size_t n_nodes,
                            BcodeBuffer *b, ConstantPoolCreator *p) {
  for (size_t k = 0; k < n_nodes; ++k)
    instance_compile_node(i, nodes[k], b, p);
}

void instance_compile_keyword_node_method(Instance *i, AstNode n,
                                          BcodeBuffer *b,
                                          ConstantPoolCreator *p) {
  int has_proto = FALSE, has_params = FALSE, has_name = FALSE;
  AstNode f_proto, f_body, f_params;
  AstNode f_name;

  has_proto = n.n_children > 1;
  if (has_proto) {
    f_proto = n.children[0];
    f_body = n.children[1];

    // Check if the function prototype contains
    // a parameter list or a name or both.
    AstNode t = f_proto;
    if (t.type == NODE_TYPE_TOKEN) {
      has_name = TRUE;
      f_name = t;
    } else if (t.type == NODE_TYPE_SEPARATOR || t.type == NODE_TYPE_CONTAINER) {
      has_params = TRUE;
      f_params = t;
    } else if (t.type == NODE_TYPE_CONT_CALL) {
      has_params = has_name = TRUE;
      f_name = t.children[0];
      f_params = t.children[1];
    } else {
      // error
    }

  } else {
    f_body = n.children[0];
  }

  // Compile the prototype of the function.
  bcode_buffer_append(b, OPCODE_BLOCK_IMPLICIT_START, OPCODE_SIZE);
  // Temporarily set the offset as 0.
  size_t offset = 0;
  bcode_buffer_append(b, (const uint8_t *)&offset, sizeof(size_t));
  // Pin a label to change this offset after the body has been
  // compiled. The label will also be used for calculating the
  // offset itself.
  BcodeBufferLabel bl = bcode_buffer_pin_label(b);

  if (has_proto && has_params) {
    AstNode *params;
    size_t n_params = 0;
    if (f_params.type == NODE_TYPE_CONTAINER) {
      // For now only a, b, c, ... format is supported.
      if (f_params.n_children &&
          f_params.children[0].type == NODE_TYPE_SEPARATOR) {
        params = f_params.children[0].children;
        n_params = f_params.children[0].n_children;
      } else {
        params = NULL;
        n_params = 0;
      }
    } else {
      params = f_params.children;
      n_params = f_params.n_children;
    }

    for (size_t k = 0; k < n_params; ++k) {
      AstNode param = params[k];
      if (param.type == NODE_TYPE_TOKEN) {
        Constant t;
        t.type = CONSTANT_TYPE_STRING;
        t.s_value = param.value.value;

        CONSTANT_ID id = constant_pool_creator_append_constant(p, t);

        bcode_buffer_append(b, OPCODE_GET_NULL, OPCODE_SIZE);
        bcode_buffer_append(b, OPCODE_SET_SCOPE, OPCODE_SIZE);
        bcode_buffer_append(b, (const uint8_t *)&id, sizeof(CONSTANT_ID));
      } else {
        instance_compile_node(i, param, b, p);
      }
    }
  }

  // Pin another label before the ending opcode.
  BcodeBufferLabel bu = bcode_buffer_pin_label(b);
  bcode_buffer_append(b, OPCODE_BLOCK_IMPLICIT_END, OPCODE_SIZE);

  // Calculate the offset.
  offset = bcode_buffer_label_calculate_offset(bl, bu);

  // Insert the offset in the fragment after
  // the starting opcode.
  *((size_t *)(bl.iter->fragment.ptr)) = offset;

  // Compile the body of the function.
  bcode_buffer_append(b, OPCODE_BLOCK_NOEXEC_START, OPCODE_SIZE);
  // Temporarily set the offset as 0.
  offset = 0;
  bcode_buffer_append(b, (const uint8_t *)&offset, sizeof(size_t));
  // Pin a label to change this offset after the body has been
  // compiled. The label will also be used for calculating the
  // offset itself.
  bl = bcode_buffer_pin_label(b);

  AstNode *body_children;
  size_t n_body_children;
  if (f_body.type == NODE_TYPE_CONTAINER &&
      string_eqc(f_body.value.value, "{")) {
    // error
    body_children = f_body.children;
    n_body_children = f_body.n_children;
  } else {
    n_body_children = 1;
    body_children = &f_body;
  }

  for (size_t k = 0; k < n_body_children; ++k) {
    instance_compile_node(i, body_children[k], b, p);
  }

  // Implicitly adds a 'return null' statement before the ending
  // opcode since all methods are required to return some value.
  bcode_buffer_append(b, OPCODE_RETURN_NULL, OPCODE_SIZE);

  // Pin another label before the ending opcode.
  bu = bcode_buffer_pin_label(b);
  bcode_buffer_append(b, OPCODE_BLOCK_NOEXEC_END, OPCODE_SIZE);

  // Calculate the offset.
  offset = bcode_buffer_label_calculate_offset(bl, bu);
  // Insert the offset in the fragment after
  // the starting opcode.
  *((size_t *)(bl.iter->fragment.ptr)) = offset;

  // Create the function.
  bcode_buffer_append(b, OPCODE_METHOD_CREATE, OPCODE_SIZE);

  // Store the function in a variable if the
  // variable name if the method is a named method.
  if (has_proto && has_name) {
    Constant c_name;
    c_name.s_value = f_name.value.value;
    c_name.type = CONSTANT_TYPE_STRING;
    CONSTANT_ID id = constant_pool_creator_append_constant(p, c_name);

    bcode_buffer_append(b, OPCODE_SET_SCOPE, OPCODE_SIZE);
    bcode_buffer_append(b, (const uint8_t *)&id, sizeof(CONSTANT_ID));
  }
}

void instance_compile_keyword_node_if(Instance *i, AstNode n, BcodeBuffer *b,
                                      ConstantPoolCreator *p) {
  MetaDataIf m = *(MetaDataIf *)n.meta_data;

  // Bytecode representation of an if/elif block.
  // 1. The condition is compiled first.
  // 2. If the result of the condition is
  //    false or null then jump to the
  //    end of the body using the jump.if command.
  // 4. Pin a label at the end of the condition expression.
  // 5. Compile the body.
  // 6. Pin a label at the end of the body.
  // 7. Use the previous and this label to calculate the offset
  //    from the previous block.

  // Combines the if and elif nodes into one single array
  // of AstNodes.
  size_t n_blocks = m.n_elif + 1;
  AstNode *blocks = calloc(n_blocks, sizeof(AstNode));
  blocks[0] = m.if_node;
  memcpy(blocks + 1, m.elif_nodes, m.n_elif * sizeof(AstNode));

  BcodeBufferLabel *j = calloc(n_blocks, sizeof(BcodeBufferLabel));
  size_t n_j = 0;

  for (size_t k = 0; k < n_blocks; ++k) {
    AstNode c = blocks[k].children[0];
    AstNode body = blocks[k].children[1];
    BcodeBufferLabel bl, bu;

    instance_compile_node(i, c, b, p);
    // If condition fails jump to the next block if
    // present
    bcode_buffer_append(b, OPCODE_JUMP_NIF, OPCODE_SIZE);
    size_t offset = 0;
    bcode_buffer_append(b, (const uint8_t *)&offset, sizeof(size_t));

    bl = bcode_buffer_pin_label(b);

    // Compile the body
    if (body.type == NODE_TYPE_CONTAINER)
      instance_compile_nodes(i, body.children, body.n_children, b, p);
    else
      instance_compile_node(i, body, b, p);

    size_t t = 0;

    // Add the jump statement to the end of the
    // if-elif-else block.
    bcode_buffer_append(b, OPCODE_JUMP, OPCODE_SIZE);
    bcode_buffer_append(b, (const uint8_t *)&t, sizeof(size_t));

    bu = bcode_buffer_pin_label(b);

    j[n_j++] = bu;

    offset = bcode_buffer_label_calculate_offset(bl, bu);
    memcpy(bl.iter->fragment.ptr, &offset, sizeof(size_t));
  }

  if (m.has_else) {
    AstNode body = m.else_node.children[0];
    if (body.type == NODE_TYPE_CONTAINER)
      instance_compile_nodes(i, body.children, body.n_children, b, p);
    else
      instance_compile_node(i, body, b, p);
  }

  BcodeBufferLabel be = bcode_buffer_pin_label(b);
  for (size_t k = 0; k < n_j; ++k) {
    BcodeBufferLabel l = j[k];
    size_t o = bcode_buffer_label_calculate_offset(l, be);
    *(size_t *)l.iter->fragment.ptr = o;
  }

  free(blocks);
  free(j);
}

void instance_compile_keyword_node_while(Instance *i, AstNode n, BcodeBuffer *b, ConstantPoolCreator *p) {
  AstNode c = n.children[0];
  AstNode body = n.children[1];

  size_t t = 0;

  // Pin a label before the condition.
  BcodeBufferLabel l_start = bcode_buffer_pin_label(b);
  // Compile the condition
  instance_compile_node(i, c, b, p);
  // jump.nif to end of loop if condition is not satisfied.
  bcode_buffer_append(b, OPCODE_JUMP_NIF, OPCODE_SIZE);
  bcode_buffer_append(b, (uint8_t*)&t, sizeof(size_t));

  BcodeBufferLabel l_mid = bcode_buffer_pin_label(b);

  // Compile the body.
  if (body.type == NODE_TYPE_CONTAINER && string_eqc(body.value.value, "{")) {
    instance_compile_nodes(i, body.children, body.n_children, b, p);
  } else {
    instance_compile_node(i, body, b, p);
  }

  BcodeBufferLabel l_prologue = bcode_buffer_pin_label(b);

  // jump.back to condition.
  bcode_buffer_append(b, OPCODE_JUMP_BACK, OPCODE_SIZE);
  bcode_buffer_append(b, (uint8_t*)&t, sizeof(size_t));

  // Pin a label after the end of the loop.
  BcodeBufferLabel l_end = bcode_buffer_pin_label(b);

  // Prologue to Start offset.
  size_t es_offset = bcode_buffer_label_calculate_offset(l_start, l_prologue);

  // Mid to End offset.
  size_t me_offset = bcode_buffer_label_calculate_offset(l_mid, l_end);

  *((size_t*)l_end.iter->fragment.ptr) = es_offset;
  *((size_t*)l_mid.iter->fragment.ptr) = me_offset;
}

void instance_compile_keyword_node_class(Instance *i, AstNode n, BcodeBuffer *b, ConstantPoolCreator *p) {
  AstNode name = n.children[0];
  AstNode inherits, body;
  int has_super = n.n_children == 3;

  if (has_super) {
    inherits = n.children[1];
    body = n.children[2];
  } else {
    body = n.children[1];
  }

  // Take care of base classes.
  if (has_super) {
    instance_compile_node(i, inherits, b, p);
  } else {
    bcode_buffer_append(b, OPCODE_GET_NULL, OPCODE_SIZE);
  }

  // Compile the body
  instance_compile_node(i, body, b, p);

  // Compile the name of the class.
  if (name.type == NODE_TYPE_TOKEN) {   
    instance_compile_node_as_literal(i, name, b, p);
  } else {
    instance_compile_node(i, name, b, p);
  }

  // Classify the object.
  bcode_buffer_append(b, OPCODE_CLASSIFY, OPCODE_SIZE);

  // Don't recompile the node again since 
  // 'classify' doesnot remove the class name 
  // from the stack.
  // instance_compile_node(i, name, b, p);

  bcode_buffer_append(b, OPCODE_SET_SCOPE_STACK, OPCODE_SIZE);
}

void instance_compile_keyword_node(Instance *i, AstNode n, BcodeBuffer *b,
                                   ConstantPoolCreator *p) {
  Token v = n.value;
  if (string_eqc(v.value, "let")) {

  } else if (string_eqc(v.value, "return")) {
    if (n.n_children) {
      instance_compile_node(i, n.children[0], b, p);
      bcode_buffer_append(b, OPCODE_RETURN, OPCODE_SIZE);
    } else {
      bcode_buffer_append(b, OPCODE_RETURN_NULL, OPCODE_SIZE);
    }
  } else if (string_eqc(v.value, "method")) {
    instance_compile_keyword_node_method(i, n, b, p);
  } else if (string_eqc(v.value, "if")) {
    instance_compile_keyword_node_if(i, n, b, p);
  } else if (string_eqc(v.value, "while")) {
    instance_compile_keyword_node_while(i, n, b, p);
  } else if (string_eqc(v.value, "class")) {
    instance_compile_keyword_node_class(i, n, b, p);
  }
}



void instance_compile_pattern_node(Instance *i, AstNode n, BcodeBuffer *b,
                                   ConstantPoolCreator *p) {
  if (n.type == NODE_TYPE_CONTAINER) {

  } else if (n.type == NODE_TYPE_TOKEN) {
    instance_compile_node_as_literal(i, n, b, p);
  } else if (n.type == NODE_TYPE_BINARY_OPERATOR) {
    if (string_eqc(n.value.value, ".")) {
      instance_compile_node(i, n.children[0], b, p);
      instance_compile_pattern_node(i, n.children[1], b, p);
    }
  }
}

void instance_compile_pattern_matcher(Instance *i, AstNode n, BcodeBuffer *b,
                                      ConstantPoolCreator *p) {
  AstNode pattern = n.children[0];
  AstNode val = n.children[1];

  // Compile rvalue expression.
  instance_compile_node(i, val, b, p);

  if (pattern.type == NODE_TYPE_TOKEN) {
    Constant c;
    c.type = CONSTANT_TYPE_STRING;
    c.s_value = pattern.value.value;
    CONSTANT_ID id;
    if (!constant_pool_creator_search(p, c, &id)) {
      id = constant_pool_creator_add_constant(p, c);
    }

    bcode_buffer_append(b, OPCODE_SET_SCOPE, OPCODE_SIZE);
    bcode_buffer_append(b, (const uint8_t *)&id, sizeof(CONSTANT_ID));
  } else {
    instance_compile_pattern_node(i, pattern, b, p);
    bcode_buffer_append(b, OPCODE_SET_OBJECT, OPCODE_SIZE);
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

void instance_compile_cont_call(Instance *i, AstNode n, BcodeBuffer *b,
                                ConstantPoolCreator *p) {
  // hello (a, b)
  // Here 'hello' is the source node (src)
  // Here (a, b) is the container node (cont)
  AstNode src_n = n.children[0];
  AstNode cont_n = n.children[1];

  // If container node is '(', then the cont call
  // is a function call.
  // If container node is '[', then the cont call
  // is an array access operation.

  // Function Definitions
  // -------------------------
  // 1. cont_n must be '{'
  // 2. src_n can be any of the following :-
  //      (a) Can be another cont-call with
  //         its <source> being an identifier and
  //         <cont> being an
  //
  //      (b) Can be an identifier token. In such
  //          a case the function does not take any paramter.
  if (string_eqc(cont_n.value.value, "(")) {
    // Compile the parameters and push them onto the stack.
    FzInt param_count = 0;

    if (cont_n.children) {
      for (size_t j = 0; j < cont_n.n_children; ++j) {
        AstNode c = cont_n.children[j];
        if (c.type == NODE_TYPE_SEPARATOR) {
          for (size_t k = 0; k < c.n_children; ++k, ++param_count)
            instance_compile_node(i, c.children[k], b, p);
        } else {
          instance_compile_node(i, c, b, p);
          param_count++;
        }
      }
    }
    // Compile the node that will put the function to be called on the stack.
    instance_compile_node(i, src_n, b, p);
    bcode_buffer_append(b, OPCODE_INVOKE_CONSTANT, OPCODE_SIZE);
    bcode_buffer_append(b, (const uint8_t *)&param_count, sizeof(FzInt));
  }
}


void instance_compile_operator_node_and_or(Instance *i, AstNode n, BcodeBuffer *b, ConstantPoolCreator *p) {
  AstNode l, r;
  l = n.children[0];
  r = n.children[1];

  size_t t =0;

  instance_compile_node(i, l, b, p);

  if (string_eqc(n.value.value, "and")) {
    bcode_buffer_append(b, OPCODE_JUMP_NIF_NOPOP, OPCODE_SIZE);
  } else {
    bcode_buffer_append(b, OPCODE_JUMP_IF_NOPOP, OPCODE_SIZE);
  }

  bcode_buffer_append(b, (uint8_t*)&t, sizeof(size_t));

  BcodeBufferLabel l_l = bcode_buffer_pin_label(b);

  bcode_buffer_append(b, OPCODE_POP, OPCODE_SIZE);

  instance_compile_node(i, r, b, p);

  BcodeBufferLabel l_r = bcode_buffer_pin_label(b);

  *((size_t*)l_l.iter->fragment.ptr) = bcode_buffer_label_calculate_offset(l_l, l_r);
}


void instance_compile_node(Instance *i, AstNode n, BcodeBuffer *b,
                           ConstantPoolCreator *p) {
  Token v = n.value;
  if (n.type == NODE_TYPE_BINARY_OPERATOR) {
    if (string_eqc(v.value, "=")) {

      instance_compile_pattern_matcher(i, n, b, p);

    } else if (string_eqc(v.value, "<-")) {

    } else if (string_eqc(v.value, "and") || string_eqc(v.value, "or")) {
      instance_compile_operator_node_and_or(i, n, b, p);

    } else if (string_eqc(v.value, ".")) {

      instance_compile_node(i, n.children[0], b, p);

      if (n.children[1].type != NODE_TYPE_TOKEN) {
        // error: key must be a literal or an identifier
      }

      AstNode key = n.children[1];
      instance_compile_node_as_literal(i, key, b, p);

      bcode_buffer_append(b, OPCODE_GET_OBJECT, OPCODE_SIZE);

    } else {
      instance_compile_node(i, n.children[0], b, p);
      instance_compile_node(i, n.children[1], b, p);

      const uint8_t *opcode = _map_opcode(v.value);
      bcode_buffer_append(b, opcode, OPCODE_SIZE);
    }
  } else if (n.type == NODE_TYPE_UNARY_OPERATOR) {

    instance_compile_node(i, n.children[0], b, p);
    const uint8_t *opcode = _map_opcode(v.value);
    bcode_buffer_append(b, opcode, OPCODE_SIZE);

  } else if (n.type == NODE_TYPE_CONTAINER) {
    if (string_eqc(v.value, "{")) {
      const uint8_t *s_op, *e_op;
      s_op = OPCODE_BLOCK_IMPLICIT_START;
      e_op = OPCODE_BLOCK_IMPLICIT_END;

      bcode_buffer_append(b, s_op, OPCODE_SIZE);

      // Temporarily set the offset as 0.
      size_t offset = 0;
      bcode_buffer_append(b, (const uint8_t *)&offset, sizeof(size_t));

      BcodeBufferLabel bl = bcode_buffer_pin_label(b);

      // Compile the body of the container.
      for (size_t j = 0; j < n.n_children; ++j) {
        instance_compile_node(i, n.children[j], b, p);
      }

      BcodeBufferLabel bu = bcode_buffer_pin_label(b);
      bcode_buffer_append(b, e_op, OPCODE_SIZE);

      offset = bcode_buffer_label_calculate_offset(bl, bu);
      *((size_t *)(bl.iter->fragment.ptr)) = offset;

    } else if (string_eqc(n.value.value, "(")) {
      // Compile the body of the container.
      instance_compile_nodes(i, n.children, n.n_children, b, p);
    }
  } else if (n.type == NODE_TYPE_KEYWORD) {

    instance_compile_keyword_node(i, n, b, p);

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
