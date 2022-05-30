#include "runtime/compiler.h"
#include "pre/ast.h"
#include "pre/meta.h"
#include "pre/tok.h"
#include "runtime/bcode.h"
#include "runtime/const_pool.h"
#include "runtime/obj.h"
#include "str.h"

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
  __OPCODE_MAPPER("!", OPCODE_NOT);
  __OPCODE_MAPPER("proto", OPCODE_PROTO);
  return NULL;
}

int is_token_literal(Token t) {
  switch (t.type) {
    case TOKEN_TYPE_LITERAL_INT_DECIMAL:
    case TOKEN_TYPE_LITERAL_INT_BINARY:
    case TOKEN_TYPE_LITERAL_INT_OCTAL:
    case TOKEN_TYPE_LITERAL_INT_HEX:
    case TOKEN_TYPE_LITERAL_FLOAT_DECIMAL:
    case TOKEN_TYPE_LITERAL_BOOLEAN:
    case TOKEN_TYPE_LITERAL_STRING:
    case TOKEN_TYPE_LITERAL_NULL:
      return TRUE;
    default:
      return FALSE;
  }
}

int parse_complex_string(string s) {
  // Complex strings are written within "" quotes
  // In order to embed values within a complex
  // string use { ... } paranthesis.
  // Example: "my name is {me.name} and age is {me.age}"
  // You can skip a { using \{.
  int f = 0;
  for (size_t i=1; i < s.length-1; ++i) {
    char c = s.value[i];
    if (c == '{') {

    } else if (c == '\\') {

    } else {

    }
  }

  return -1;
}

void instance_compile_complex_string(Instance *i, AstNode n, BcodeBuffer *b, ConstantPoolCreator *p) {
  parse_complex_string(n.value.value);
}


Constant token_to_constant(Token t) {
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

  return c;
}

void instance_compile_literal(Instance *i, AstNode n, BcodeBuffer *b,
                              ConstantPoolCreator *p) {
  Token t = n.value;

  if (t.type == TOKEN_TYPE_LITERAL_NULL) {
    bcode_buffer_append(b, OPCODE_GET_NULL, OPCODE_SIZE);
    return;
  }

  Constant c = token_to_constant(t);
  
  // Add it to the constant pool creator and retrieve the
  // constant id.
  CONSTANT_ID id;
  if (!constant_pool_creator_search(p, c, &id)) {
    id = constant_pool_creator_add_constant(p, c);
  }

  bcode_buffer_append(b, OPCODE_GET_CONSTANT, OPCODE_SIZE);
  bcode_buffer_append(b, (const uint8_t *)&id, sizeof(id));

  //return c;
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
      if (f_params.n_children) {
        if (f_params.children[0].type == NODE_TYPE_SEPARATOR) {
          params = f_params.children[0].children;
          n_params = f_params.children[0].n_children;
        } else {
          params = f_params.children;
          n_params = f_params.n_children;
        }
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

void instance_compile_keyword_node_use(Instance *i, AstNode n, BcodeBuffer *b, ConstantPoolCreator *p) {
  // Check if the operand is an 'as' expression.
  // 'as' is used to provide to lend an alias to the
  // imported module.

  AstNode as = n.children[0];

  AstNode path, alias;

  int has_alias = FALSE;

  if (as.type == NODE_TYPE_BINARY_OPERATOR && string_eqc(as.value.value, "as")) {
    has_alias = TRUE;
    path = as.children[0];
    alias = as.children[1];
  } else {
    path = as;
  }

  instance_compile_node(i, path, b, p);
  bcode_buffer_append(b, OPCODE_MODULE_LOAD, OPCODE_SIZE);

  if (has_alias) {
    instance_compile_node(i, alias, b, p);
    bcode_buffer_append(b, OPCODE_SET_SCOPE_STACK, OPCODE_SIZE);
  }
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
  } else if (string_eqc(v.value, "use")) {
    instance_compile_keyword_node_use(i, n, b, p);
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

size_t instance_compile_func_call_params(Instance *i, AstNode *children, size_t n_children, 
  BcodeBuffer *b, ConstantPoolCreator *p) {

  FzInt param_count = 0;

  if (children) {
    for (size_t j = 0; j < n_children; ++j) {
      AstNode c = children[j];
      if (c.type == NODE_TYPE_SEPARATOR) {
        for (size_t k = 0; k < c.n_children; ++k, ++param_count)
          instance_compile_node(i, c.children[k], b, p);
      } else {
        instance_compile_node(i, c, b, p);
        param_count++;
      }
    }
  }

  return param_count;
}

void instance_compile_method_call(Instance *i, AstNode n, BcodeBuffer *b,
                          ConstantPoolCreator *p) {
  // Expression is of the form abc(a, b)
  // where,
  //     Left Node = 'abc' (index 0) [Can be any other node]
  //     Right Node = '(a, b)' (index 1)

  // src_n <= Left Node
  AstNode src_n = n.children[0];
  // cont_n <= Right Node
  AstNode cont_n = n.children[1];

  // Compile the parameters and push them onto the stack.
  FzInt param_count = instance_compile_func_call_params(
        i, cont_n.children, cont_n.n_children, b, p);    

  // Check if the node has meta data.
  MetaDataContCall *m = n.meta_data;

  // Compile the node that will put the function to be called on the stack.
  instance_compile_node(i, src_n, b, p);
  
  if(m && m->detached) {
    bcode_buffer_append(b, OPCODE_INVOKE_CONSTANT_ASYNC, OPCODE_SIZE);
  } else {
    bcode_buffer_append(b, OPCODE_INVOKE_CONSTANT, OPCODE_SIZE);
  }

  bcode_buffer_append(b, (const uint8_t *)&param_count, sizeof(FzInt));
}


void instance_compile_object_method_call(Instance *i, AstNode n, BcodeBuffer *b, 
                          ConstantPoolCreator *p) {
  // Expression is of the form dog.bark(a, b)
  // Left Node -> 'dog.bark' (index 0)
  //       |
  //       |----> dog (index 0)
  //       |
  //       |----> bark (index 1)       
  //
  // Right Node -> '(a, b)' (index 1) 
  // The result of the Left Node will be the 
  // treated as the 'me' for the method call.

  AstNode fn = n.children[0];
  AstNode cont_n = n.children[1];

  // Compile the parameters.
  FzInt n_params = instance_compile_func_call_params(i, cont_n.children,
            cont_n.n_children, b, p);

  int is_detached = n.meta_data && ((MetaDataContCall*)n.meta_data)->detached;

  // Push the object onto the stack
  instance_compile_node(i, fn.children[0], b, p);

  // Push the function name onto the stack.
  instance_compile_node_as_literal(i, fn.children[1], b, p);

  if (is_detached) {
    bcode_buffer_append(b, OPCODE_INVOKE_CONSTANT_ME_ASYNC, OPCODE_SIZE);
  } else {
    bcode_buffer_append(b, OPCODE_INVOKE_CONSTANT_ME, OPCODE_SIZE);
  }

  bcode_buffer_append(b, (uint8_t*)&n_params, sizeof(FzInt));
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
    
    if (src_n.type == NODE_TYPE_BINARY_OPERATOR &&
          string_eqc(src_n.value.value, ".")) {
      instance_compile_object_method_call(i, n, b, p);
    } else {
      instance_compile_method_call(i, n, b, p);
    }
  
  } else if (string_eqc(cont_n.value.value, "[")) {
    // Array element access scenario. The element
    // is fetched by implicitly calling the .__slicer()
    // function.
    FzInt count = 0;
    for (int k=0; k < cont_n.n_children; ++k) {
      AstNode c = cont_n.children[k];
      if (c.type == NODE_TYPE_SEPARATOR) {
        for (int j=0; j < c.n_children; ++j, ++count) {
          instance_compile_node(i, c.children[j], b, p);
        }
      } else {
        instance_compile_node(i, c, b, p);
        ++count;
      }
    }

    // The object.
    instance_compile_node(i, src_n, b, p);

    // The method name which is __slicer.
    Constant c;
    c.s_value = to_string("__slicer");
    c.type = CONSTANT_TYPE_STRING;

    CONSTANT_ID cid = constant_pool_creator_append_constant(p, c);
    bcode_buffer_append(b, OPCODE_GET_CONSTANT, OPCODE_SIZE);
    bcode_buffer_append(b, (uint8_t*)&cid, sizeof(CONSTANT_ID));

    bcode_buffer_append(b, OPCODE_INVOKE_CONSTANT_ME, OPCODE_SIZE);
    bcode_buffer_append(b, (uint8_t*)&count, sizeof(FzInt));
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

void instance_compile_operator_new(Instance *i, AstNode n, BcodeBuffer *b, ConstantPoolCreator *p) {
  AstNode x = n.children[0];

  AstNode name;
  AstNode *c = NULL;
  size_t n_c = 0; 

  if (x.type == NODE_TYPE_CONT_CALL) {
    name = x.children[0];
    c = x.children[1].children;
    n_c = x.children[1].n_children;
  } else {
    name = x;
  }

  FzInt n_params = (FzInt)instance_compile_func_call_params(i, c, n_c, b, p);
  instance_compile_node(i, name, b, p);

  bcode_buffer_append(b, OPCODE_INSTANTIATE, OPCODE_SIZE);
  bcode_buffer_append(b, (uint8_t*)&n_params, sizeof(FzInt));

  // Get the 'new' property from tne object and
  // call it.
  Constant nw;
  nw.type = CONSTANT_TYPE_STRING;
  nw.s_value = to_string("new");

  CONSTANT_ID cid = constant_pool_creator_append_constant(p, nw); 
  bcode_buffer_append(b, OPCODE_GET_CONSTANT, OPCODE_SIZE);
  bcode_buffer_append(b, (uint8_t*)&cid, sizeof(CONSTANT_ID));

  bcode_buffer_append(b, OPCODE_INVOKE_CONSTANT_ME, OPCODE_SIZE);
  bcode_buffer_append(b, (uint8_t*)&n_params, sizeof(FzInt));
}

void instance_compile_operator_unary_minus(Instance *i, AstNode n, BcodeBuffer *b, ConstantPoolCreator *p) {
  AstNode c = n.children[0];
  Token t = c.value;
  string v = t.value;

  if (c.type == NODE_TYPE_TOKEN && is_token_literal(c.value)) {
    string s;
    s.length = v.length + 1;
    char *ptr = malloc(s.length);
    ptr[0] = '-';
    memcpy(ptr + 1, v.value, v.length);
    s.value = ptr;

    Token t;
    t.type = c.value.type;
    t.value = s;

    Constant c = token_to_constant(t);
    CONSTANT_ID cid = constant_pool_creator_append_constant(p, c);

    bcode_buffer_append(b, OPCODE_GET_CONSTANT, OPCODE_SIZE);
    bcode_buffer_append(b, (uint8_t*)&cid, sizeof(CONSTANT_ID));

    free(ptr);
  } else {
    Constant zero;
    zero.type = CONSTANT_TYPE_INT;
    zero.i_value = 0;

    CONSTANT_ID cid = constant_pool_creator_append_constant(p, zero);

    bcode_buffer_append(b, OPCODE_GET_CONSTANT, OPCODE_SIZE);
    bcode_buffer_append(b, (uint8_t*)&cid, sizeof(CONSTANT_ID));

    instance_compile_node(i, c, b, p);

    bcode_buffer_append(b, OPCODE_SUB, OPCODE_SIZE);
  }
}

void instance_compile_operator_dep(Instance *i, AstNode n, BcodeBuffer *b, ConstantPoolCreator *p) {
  
  AstNode dy = n.children[0], dt = n.children[1];
  // When it comes to the dependant, it must be a block.
  // The block must be a {...} container.
  // In case of single line, blocks, the paranthesis can 
  // be omitted.
  size_t offset = 0;

  bcode_buffer_append(b, OPCODE_BLOCK_NOEXEC_IMPLICIT_START, OPCODE_SIZE);
  bcode_buffer_append(b, (uint8_t*)&offset, sizeof(size_t));

  BcodeBufferLabel s = bcode_buffer_pin_label(b);

  if (dt.type == NODE_TYPE_CONTAINER && 
        string_eqc(dt.value.value, "{")) {
    instance_compile_nodes(i, dt.children, dt.n_children, b, p);
  } else {
    instance_compile_node(i, dt, b, p);
  }

  BcodeBufferLabel e = bcode_buffer_pin_label(b);
  bcode_buffer_append(b, OPCODE_BLOCK_NOEXEC_IMPLICIT_END, OPCODE_SIZE);

  offset = bcode_buffer_label_calculate_offset(s, e);

  *(size_t*)(s.iter->fragment.ptr) = offset;

  // The left operand  will be the dependency for 
  // the second operand(on the right which is the dependant.

  size_t n_dy = 1;

  // If dy is of the form ( 1, 2, .... ) then it indicates
  // the presence of multiple dependencies.
  if (dy.type == NODE_TYPE_CONTAINER && 
        string_eqc(dy.value.value, "(")) {
    
    size_t count = 0;
    for (size_t l=0; l < dy.n_children; ++l) {
      AstNode c = dy.children[l];
      if (c.type == NODE_TYPE_SEPARATOR) {
        count += c.n_children;

        for (size_t j=0; j < c.n_children; ++j) {
          instance_compile_pattern_node(i, c.children[j], b, p);
          
          if (c.children[j].type == NODE_TYPE_TOKEN) {
            bcode_buffer_append(b, OPCODE_SET_SCOPE_DEPENDANT_STACK, OPCODE_SIZE);
          } else {
            bcode_buffer_append(b, OPCODE_SET_OBJECT_DEPENDANT, OPCODE_SIZE);
          }

        }

      } else {
        instance_compile_pattern_node(i, c, b, p);
        
        if (c.type == NODE_TYPE_TOKEN) {
          bcode_buffer_append(b, OPCODE_SET_SCOPE_DEPENDANT_STACK, OPCODE_SIZE);
        } else {
          bcode_buffer_append(b, OPCODE_SET_OBJECT_DEPENDANT, OPCODE_SIZE);
        }

        ++count;
      }

    }
  } else {
    instance_compile_pattern_node(i, dy, b, p);
    
    if (dy.type == NODE_TYPE_TOKEN) {
      bcode_buffer_append(b, OPCODE_SET_SCOPE_DEPENDANT_STACK, OPCODE_SIZE);
    } else {
      bcode_buffer_append(b, OPCODE_SET_OBJECT_DEPENDANT, OPCODE_SIZE);
    }
  }
}

void instance_compile_operator_attach(Instance *i, AstNode n,
            BcodeBuffer *b, ConstantPoolCreator *p) {

  AstNode target = n.children[0];
  AstNode payload = n.children[1];

  // Compile the payload.
  instance_compile_node(i, payload, b, p);

  // Target can either be of the form x.y
  // or must be an identifier.

  if (target.type == NODE_TYPE_BINARY_OPERATOR && 
        string_eqc(target.value.value, ".")) {
    instance_compile_node(i, target.children[0], b, p);
    instance_compile_node_as_literal(i, target.children[1], b, p);

    bcode_buffer_append(b, OPCODE_ATTACH_OBJECT, OPCODE_SIZE);
  } else if (target.type == NODE_TYPE_TOKEN && 
          target.value.type == TOKEN_TYPE_IDENTIFIER) {
    instance_compile_node_as_literal(i, target.children[0], b, p);
    bcode_buffer_append(b, OPCODE_ATTACH_SCOPE, OPCODE_SIZE);
  } else {
    // error
  }
}

void instance_compile_method_param_list(Instance *i, AstNode *c, size_t nc, 
            BcodeBuffer *b, ConstantPoolCreator *p) {
  size_t offset = 0;

  bcode_buffer_append(b, OPCODE_BLOCK_IMPLICIT_START, OPCODE_SIZE);

  bcode_buffer_append(b, (uint8_t*)&offset, sizeof(size_t));

  BcodeBufferLabel l = bcode_buffer_pin_label(b);

  // Count the number of parameters.
  FzInt n_params = 0;
  for (int m=0; m < nc; ++m) {
    AstNode y = c[m];
    if (y.type == NODE_TYPE_TOKEN && 
          y.value.type == TOKEN_TYPE_IDENTIFIER) {
      bcode_buffer_append(b, OPCODE_GET_NULL, OPCODE_SIZE);
    
      Constant c;
      c.type = CONSTANT_TYPE_STRING;
      c.s_value = y.value.value;
    
      CONSTANT_ID cid = constant_pool_creator_append_constant(p, c);
    
      bcode_buffer_append(b, OPCODE_SET_SCOPE, OPCODE_SIZE);
      bcode_buffer_append(b, (uint8_t*)&cid, sizeof(CONSTANT_ID));
    } else {
      instance_compile_node(i, y, b, p);
    }
    ++n_params;
  }

  BcodeBufferLabel u = bcode_buffer_pin_label(b);

  bcode_buffer_append(b, OPCODE_BLOCK_IMPLICIT_END, OPCODE_SIZE);

  offset = bcode_buffer_label_calculate_offset(l, u);
  *(size_t*)(l.iter->fragment.ptr) = offset;
}

void instance_compile_method_body(Instance *i, AstNode n, BcodeBuffer *b,
                              ConstantPoolCreator *p) {
  size_t offset = 0;
  bcode_buffer_append(b, OPCODE_BLOCK_NOEXEC_START, OPCODE_SIZE);
  bcode_buffer_append(b, (uint8_t*)&offset, sizeof(size_t));

  BcodeBufferLabel l = bcode_buffer_pin_label(b);
  
  // If the body is not enclosed within { ... }, then
  // the result of the expression will be the return value.
  if (string_eqc(n.value.value, "{")) {
    for (int k=0; k < n.n_children; ++k) {
      instance_compile_node(i, n.children[k], b, p);
    }
    bcode_buffer_append(b, OPCODE_RETURN_NULL, OPCODE_SIZE);
  } else {
    instance_compile_node(i, n, b, p);
    bcode_buffer_append(b, OPCODE_RETURN, OPCODE_SIZE);
  }

  BcodeBufferLabel u = bcode_buffer_pin_label(b);
  
  bcode_buffer_append(b, OPCODE_BLOCK_NOEXEC_END, OPCODE_SIZE);
  offset = bcode_buffer_label_calculate_offset(l, u);
  *(size_t*)(l.iter->fragment.ptr) = offset;
}

void instance_compile_operator_closure(Instance *i, AstNode n, BcodeBuffer *b,
                          ConstantPoolCreator *p) {
  AstNode params_n, body_n;

  params_n = n.children[0];
  body_n = n.children[1];

  if (params_n.type != NODE_TYPE_CONTAINER) {
    // error
  }

  // Compile the parameter list
  if (params_n.children) {
    AstNode t = params_n.children[0];
    if (t.type == NODE_TYPE_SEPARATOR &&
          string_eqc(t.value.value, ",")) {
      instance_compile_method_param_list(i, t.children, t.n_children, b, p);
    } else {
      instance_compile_method_param_list(i, &t, 1, b, p);
    }
  } else {
    bcode_buffer_append(b, OPCODE_GET_NULL, OPCODE_SIZE);
  }

  // Compile the body
  instance_compile_method_body(i, body_n, b, p);

  // Create the method
  bcode_buffer_append(b, OPCODE_METHOD_CREATE, OPCODE_SIZE);
}



void instance_compile_node(Instance *i, AstNode n, BcodeBuffer *b,
                           ConstantPoolCreator *p) {
  Token v = n.value;
  if (n.type == NODE_TYPE_BINARY_OPERATOR) {
    if (string_eqc(v.value, "=")) {

      instance_compile_pattern_matcher(i, n, b, p);

    } else if (string_eqc(v.value, "<-")) {
      instance_compile_operator_dep(i, n, b, p);

    } else if(string_eqc(v.value, "->")) {
      instance_compile_operator_closure(i, n, b, p);

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
    
    } else if (string_eqc(v.value, ":")) {
      instance_compile_operator_attach(i, n, b, p);

    } else {
      instance_compile_node(i, n.children[0], b, p);
      instance_compile_node(i, n.children[1], b, p);

      const uint8_t *opcode = _map_opcode(v.value);
      bcode_buffer_append(b, opcode, OPCODE_SIZE);
    }
  } else if (n.type == NODE_TYPE_UNARY_OPERATOR) {

    if (string_eqc(v.value, "new")) {
      instance_compile_operator_new(i, n, b, p);
    } else if (string_eqc(v.value, "-")) {
      instance_compile_operator_unary_minus(i, n, b, p);
    } else {
      instance_compile_node(i, n.children[0], b, p);
      const uint8_t *opcode = _map_opcode(v.value);
      bcode_buffer_append(b, opcode, OPCODE_SIZE);
    }


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
    } else if (string_eqc(n.value.value, "[")) {

      FzInt count = 0;
      for (int k=0; k < n.n_children; ++k) {
        AstNode c = n.children[k];
        if (c.type == NODE_TYPE_SEPARATOR) {
          for (int j=0; j < c.n_children; ++j, ++count) {
            instance_compile_node(i, c.children[j], b, p);
          }
        } else {
          instance_compile_node(i, c, b, p);
          ++count;
        }
      }

      bcode_buffer_append(b, OPCODE_LISTIFY, OPCODE_SIZE);
      bcode_buffer_append(b, (uint8_t*)&count, sizeof(FzInt));
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
    case TOKEN_TYPE_LITERAL_NULL:
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
