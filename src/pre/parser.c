#include "pre/parser.h"

#include "pre/ast.h"
#include "pre/tok.h"
#include "pre/meta.h"
#include "str.h"
#include <stdio.h>
#include <string.h>

AstNodeListIterator *get_container_end(AstNodeListIterator *i) {
  AstNodeList stack;
  ast_node_list_init(&stack);
  ast_node_list_insert_after(&stack, i->value, NULL);

  for (i = i->next; i != NULL; i = i->next) {
    if (i->value.type != NODE_TYPE_TOKEN) {
      continue;
    }

    Token t = i->value.value;
    if (is_container_opener(t.value)) {
      ast_node_list_insert_after(&stack, i->value, NULL);
    } else if (is_container_closer(t.value)) {
      AstNodeListIterator *k = ast_node_list_begin(&stack);
      AstNode n = k->value;
      if (!string_eq(container_opener_to_closer(n.value.value),
                     i->value.value.value)) {
        // invalid syntax
      }
      ast_node_list_remove(k);
    }

    if (ast_node_list_begin(&stack) == NULL) {
      break;
    }
  }
  ast_node_list_destroy(&stack);
  return i;
}

void parse_containers(AstNodeList *l, AstNodeListIterator *i,
                      AstNodeListIterator *e) {
  // printf("cont ");
  // token_info(i->value.value);
  for (; i != e; i = i->next) {
    AstNode n = i->value;
    if (n.type != NODE_TYPE_TOKEN) {
      continue;
    }

    Token t = n.value;
    if (is_container_opener(t.value)) {
      AstNodeListIterator *ce = get_container_end(i);
      if (ce == NULL) {
        // could not find end of container.
      }

      if (i->next != ce) {
        parse(l, i->next, ce);
        // Count the number of child nodes present inside the container.
        AstNodeListIterator *t = i->next;
        size_t c = 0;
        while (t != ce) {
          ++c;
          t = t->next;
        }
        // printf("count: %ld\n", c);
        AstNode *children = calloc(c, sizeof(AstNode));

        // Remove all nodes in between the the opener and closer, and
        // insert them as children of the container node.
        t = i->next;
        size_t m = 0;
        do {
          children[m++] = t->value;
          t = t->next;
          ast_node_list_remove(t->prev);
        } while (t != ce);
        n.children = children;
        n.n_children = c;
      } else {
      	n.n_children = 0;
      }

      ast_node_list_remove(ce);

      n.type = NODE_TYPE_CONTAINER;

      i->value = n;
    }
  }

  // printf("end cont\n");
}

AstNodeListIterator *parse_operator(AstNodeList *l, AstNodeListIterator *s,
                                    AstNodeListIterator *e,
                                    AstNodeListIterator *i) {
  AstNode n = i->value;
  Token o = n.value;
  if (is_binary_operator(o.value)) {

    if (i == s) {
      // could not find first operand
    }

    if (i->next == NULL || i->next == e) {
      // could not find second operand.
    }

    AstNodeListIterator *prev = i->prev, *next = i->next;
    AstNode o1 = prev->value, o2 = next->value;

    n.type = NODE_TYPE_BINARY_OPERATOR;
    n.n_children = 2;

    AstNode *children = calloc(2, sizeof(AstNode));
    children[0] = o1;
    children[1] = o2;

    n.children = children;

    ast_node_list_remove(next);
    ast_node_list_remove(i);

    prev->value = n;
    i = prev;
  } else {
    AstNodeListIterator *next = i->next;
    if (next == NULL || next == e) {
      // could not find the only operand (the operand on the right).
    }

    AstNode n = i->value;
    n.type = NODE_TYPE_UNARY_OPERATOR;
    n.n_children = 1;

    AstNode *children = calloc(1, sizeof(AstNode));
    *children = next->value;

    n.children = children;

    ast_node_list_remove(next);
    i->value = n;
  }

  return i;
}

void parse_operators(AstNodeList *l, AstNodeListIterator *s,
                     AstNodeListIterator *e) {
  /*printf("op\n");
  printf("<s> = ");
  ast_dump_node(s->value, 0);
  if (e != NULL) {
    printf("<e> = ");
    ast_dump_node(e->value, 0);
  }*/
  for (int k = 0; k < PRECEDENCE_LIST_LENGTH; ++k) {
    // printf("------------\n");
    PRECEDENCE p = PRECEDENCE_LIST[k];
    AstNodeListIterator *i, *d;
    if (p.associativity == ASSOCIATIVITY_LEFT_TO_RIGHT) {
      i = s;
      d = e;
    } else {
      d = s->prev;
      i = e == NULL ? ast_node_list_end(l) : e->prev;
    }

    while (i != d) {
      // printf("<addr i> %ld ", (size_t)i);
      /*if (i != NULL) {
        printf("%d <i> ", k);
        ast_dump_node(i->value, 0);
      }*/
      AstNode n = i->value;
      if (n.type == NODE_TYPE_TOKEN) {
        for (int j = 0; j < p.n_operators; ++j) {
          string o = to_string(p.operators[j]);
          if (string_eq(o, n.value.value)) {
            i = parse_operator(l, s, e, i);
            // ast_dump_node(i->value, 0);
          }
        }
      } else if (k == 0 && i != s && n.type == NODE_TYPE_CONTAINER) {
        // k==0 so that container calls are parsed only at the
        // precedence level of the '.' operator.
        AstNodeListIterator *prev = i->prev;
        AstNode p = prev->value;
        if (p.type != NODE_TYPE_CONTAINER && p.type != NODE_TYPE_TOKEN &&
            p.type != NODE_TYPE_BINARY_OPERATOR 
            // <expression> { ... } is not a container call.
            // examples: a.b.c() { ... } or a { ... }
            // OLD: && p.type == NODE_TYPE_CONT_CALL && string_eqc(n.value.value, "{")
            || string_eqc(n.value.value, "{"))
          goto __j;

        // previous node (p) should not be a keyword
        if (p.value.type == NODE_TYPE_KEYWORD)
          goto __j;

        if (p.type == NODE_TYPE_TOKEN) {
          Token pt = p.value;
          if (pt.type != TOKEN_TYPE_IDENTIFIER)
            goto __j;
        }

        AstNode *children = calloc(2, sizeof(AstNode));
        children[0] = p;
        children[1] = n;

        p.type = NODE_TYPE_CONT_CALL;
        p.children = children;
        p.n_children = 2;

        prev->value = p;

        ast_node_list_remove(i);
        i = prev;
      }

    __j:

      if (p.associativity == ASSOCIATIVITY_LEFT_TO_RIGHT) {
        i = i->next;
      } else {
        i = i->prev;
      }
    }
  }
  // printf("end op\n");
}

void parse_separators(AstNodeList *l, AstNodeListIterator *s,
                      AstNodeListIterator *e) {
  for (AstNodeListIterator *i = s; i != e; i = i->next) {
    AstNode n = i->value;
    if (n.type == NODE_TYPE_TOKEN && n.value.type == TOKEN_TYPE_SEPARATOR) {
      if (i == s) {
        // error.
        // example: , 1, 2, 3
      }

      n.type = NODE_TYPE_SEPARATOR;
      n.n_children = 1;
      n.children = calloc(1, sizeof(AstNode));
      n.children[0] = i->prev->value;

      i = i->prev;
      i->value = n;

      ast_node_list_remove(i->next);

      AstNodeListIterator *j = i;
      for (;;) {
        if (j->next == NULL || j->next == e) {
          break;
        }

        // Reallocate space for the children.
        AstNode *children = calloc(n.n_children + 1, sizeof(AstNode));
        memcpy(children, n.children, n.n_children * sizeof(AstNode));
        free(n.children);

        children[n.n_children++] = j->next->value;
        n.children = children;

        i->value = n;

        if (
          j->next->next != NULL &&
          j->next->next->value.type == NODE_TYPE_TOKEN && 
          j->next->next->value.value.type == TOKEN_TYPE_SEPARATOR) {
          ast_node_list_remove(j->next->next);
          ast_node_list_remove(j->next);
        } else {
          ast_node_list_remove(j->next);
          break;
        }
      }

    }
  }
}

#define KEYWORD_IS(x, y) string_eq(x.value.value, to_string(y))

void parse_keywords(AstNodeList *l, AstNodeListIterator *s,
                    AstNodeListIterator *e) {
  for (AstNodeListIterator *i = s; i != e; i = i->next) {
    AstNode n = i->value;
    if (n.type != NODE_TYPE_TOKEN)
      continue;
    if (n.value.type == TOKEN_TYPE_KEYWORD) {
      if (KEYWORD_IS(n, "say") || KEYWORD_IS(n, "return")) {
        if (i->next == e || i->next == NULL) {
          // error.
        }

        AstNode *c = calloc(1, sizeof(AstNode));
        *c = i->next->value;

        n.type = NODE_TYPE_KEYWORD;
        n.children = c;
        n.n_children = 1;

        i->value = n;
        ast_node_list_remove(i->next);
      } else if (KEYWORD_IS(n, "while")) {
        AstNode c, body;

        if (i->next == e || i->next == NULL) {
          // error: while requires a condition
        } else if (i->next->next == NULL || i->next->next == e) {
          // error: while requires a body
        }

        c = i->next->value;
        body = i->next->next->value;

        n.type = NODE_TYPE_KEYWORD;
        n.children = calloc(2, sizeof(AstNode));
        n.n_children = 2;
        n.children[0] = c;
        n.children[1] = body;

        ast_node_list_remove(i->next->next);
        ast_node_list_remove(i->next);

        i->value = n;

      } else if (KEYWORD_IS(n, "method")) {

        AstNode proto, body, *children;
        size_t n_children;

        if (i->next == e || i->next == NULL) {
          // error.
        }

        proto = i->next->value;
        
        if (proto.type == NODE_TYPE_CONTAINER && 
          string_eqc(proto.value.value, "{")) {
          body = proto;
          n_children = 1;
          children = malloc(sizeof(AstNode));
          *children = body;
        } else if (i->next->next == e || i->next->next == NULL) {
          // error
        } else {
          body = i->next->next->value;
          n_children = 2;
          children = calloc(n_children, sizeof(AstNode));
          children[0] = proto;
          children[1] = body;
        }

        n.type = NODE_TYPE_KEYWORD;
        n.children = children;
        n.n_children = n_children;

        i->value = n;

        ast_node_list_remove(i->next->next);
        ast_node_list_remove(i->next);
      } else if (KEYWORD_IS(n, "if")) {
        
        // An 'if' block requires a condition and
        // a body. 
        // After parsing the if block, look for adjacent
        // 'elif' or 'else' blocks.

        MetaDataIf m;

        AstNode *children = calloc(2, sizeof(AstNode));

        if (i->next == NULL || i->next == e) {
          // error
        }

        if (i->next->next == NULL || i->next->next == e) {
          // error
        }

        children[0] = i->next->value;
        children[1] = i->next->next->value;

        m.if_node.type = NODE_TYPE_TOKEN;
        m.if_node.value = i->value.value;
        m.if_node.children = children;
        m.if_node.n_children = 2;

        ast_node_list_remove(i->next->next);
        ast_node_list_remove(i->next);

        AstNode *elifs = NULL;
        size_t n_elifs = 0;

        while (TRUE) {
          if (i->next == NULL || i->next == e)
            break;
          AstNode x = i->next->value;
          if (x.type != NODE_TYPE_TOKEN)
            break;

          if(string_eqc(x.value.value, "elif")) {
            if (i->next->next == NULL || i->next->next == e) {
              // error: expected condition after elif
            }
            if (i->next->next->next == NULL || i->next->next->next == e) {
              // error: expected body after condition
            }

            AstNode elif = x;
            elif.type = NODE_TYPE_TOKEN;
            elif.children = calloc(2, sizeof(AstNode));
            elif.n_children = 2;
            elif.children[0] = i->next->next->value;
            elif.children[1] = i->next->next->next->value;

            AstNode *t = calloc(n_elifs + 1, sizeof(AstNode));
            if (n_elifs) {
              memcpy(t, elifs, n_elifs+1 * sizeof(AstNode));
            }
            free(elifs);
            elifs = t;

            elifs[n_elifs++] = elif;
            ast_node_list_remove(i->next->next->next);
            ast_node_list_remove(i->next->next);
            ast_node_list_remove(i->next);
          } else {
            break;
          }
        }

        m.elif_nodes = elifs;
        m.n_elif = n_elifs;

        m.has_else = FALSE;
        if (i->next != NULL && i->next != e && 
          i->next->value.type == NODE_TYPE_TOKEN && 
          string_eqc(i->next->value.value.value, "else")) {

          if (i->next->next == NULL || i->next->next == e) {
            // error: else must have a body.
          }

          m.has_else = TRUE;
          m.else_node = i->next->value;
          m.else_node.type = NODE_TYPE_TOKEN;
          m.else_node.n_children = 1;
          m.else_node.children = malloc(sizeof(AstNode));
          *m.else_node.children = i->next->next->value;

          ast_node_list_remove(i->next->next);
          ast_node_list_remove(i->next);
        }

        i->value.type = NODE_TYPE_KEYWORD;
        i->value.meta_data = malloc(sizeof(MetaDataIf));
        *(MetaDataIf*)(i->value.meta_data) = m;
      } else if (string_eqc(n.value.value, "class")) {
        
        if (i->next == NULL || i->next == e) {
          // error: expected a name and a body
        }

        AstNode name, inherits, body;
        int has_super = FALSE;

        name = i->next->value;

        if (i->next->next == NULL || i->next == e) {
          // error: expected class body
        } else if (i->next->next->value.type == NODE_TYPE_TOKEN ) {
          // TODO: Inheritance case
        } else {
          body = i->next->next->value;
          ast_node_list_remove(i->next->next);
        }

        ast_node_list_remove(i->next);

        AstNode *children;
        size_t n_children;

        if (has_super) {
          children = calloc(3, sizeof(AstNode));
          children[0] = name;
          children[1] = inherits;
          children[2] = body;
          n_children = 3;
        } else {
          children = calloc(2, sizeof(AstNode));
          children[0] = name;
          children[1] = body;
          n_children = 2;
        }

        i->value.children = children;
        i->value.n_children = n_children;
        i->value.type = NODE_TYPE_KEYWORD;
      }
    }
  }
}

void parse(AstNodeList *l, AstNodeListIterator *i, AstNodeListIterator *e) {
  // printf("parse\n");
  parse_containers(l, i, e);
  parse_operators(l, i, e);
  parse_separators(l, i, e);
  parse_keywords(l, i, e);
  // printf("end parse\n");
}