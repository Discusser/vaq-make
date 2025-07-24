#include "generator.h"
#include "array.h"
#include "generator-priv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void synchronize(vaq_make_gen *gen);
static vaq_make_token previous(vaq_make_gen *gen);
static vaq_make_token consume(vaq_make_gen *gen);
static void consume_expected(vaq_make_gen *gen, vaq_make_token_type type, const char *message);
static bool check(vaq_make_gen *gen, vaq_make_token_type type);
static bool match(vaq_make_gen *gen, vaq_make_token_type type);

static void error(vaq_make_gen *gen, const char *message);
static void error_at(vaq_make_gen *gen, vaq_make_token token, const char *message);
static void error_at_current(vaq_make_gen *gen, const char *message);

bool vaq_make_generate_build(vaq_make_scanner *scanner, const char *file_path) {
  vaq_make_gen gen;
  gen.file_name = strrchr(file_path, '/');
  if (gen.file_name == NULL)
    gen.file_name = file_path;
  else
    gen.file_name++;
  gen.scanner = scanner;
  gen.had_error = false;
  gen.panic_mode = false;
  gen.stack_size = 0;

  // TODO: Store a hash table of native functions in vaq_make_gen. This table is populated in
  // vaq_make_generate_build. Because vaq_make_generate_build is currently called for each file, and
  // multi-file support is planned, it might be wiser to make the vaq_make_gen struct encapsulate
  // multiple files. One way to do this is by having an `enclosing` field in the struct, which
  // points to the parent vaq_make_gen.
  // For each included file, a child is created. It's also important to not fall into the trap of
  // recursively reading files (recursive includes). This can be fixed by importing a file only once
  // by storing (probably absolute) file paths in a hash set and checking against the set each time
  // we read a file.
  // We can also share data across files using an `enclosing` field, simply by
  // setting child->field_to_share = child->enclosing->field_to_share. This is important for native
  // functions, but will also probably be useful for sharing variables. Design note : is it better
  // to share only variables marked with `export`, or to share variables by default, and allow a
  // `noexport` attribute or something similar. It's quite rare to have a variable that shouldn't be
  // shared in build generators. Maybe a simpler solution would be to only share global variables,
  // which would also force the notion of scope and a distiction between locals and globals.

  vaq_make_variable_array_new(&gen.variables);

  consume(&gen);
  while (!check(&gen, TOKEN_EOF)) {
    declaration(&gen);
  }
  consume_expected(&gen, TOKEN_EOF, "Expected end of expression.");

  return !gen.had_error;
}

static void synchronize(vaq_make_gen *gen) {
  gen->panic_mode = false;

  while (gen->current.type != TOKEN_EOF) {
    if (gen->previous.type == TOKEN_SEMICOLON)
      return;
    switch (gen->current.type) {
    case TOKEN_PRINT:
      return;
    default:;
    }

    consume(gen);
  }
}

static vaq_make_token previous(vaq_make_gen *gen) { return gen->previous; }

static vaq_make_token consume(vaq_make_gen *gen) {
  gen->previous = gen->current;

  while (true) {
    gen->current = vaq_make_scan_token(gen->scanner);
    if (gen->current.type != TOKEN_ERROR)
      break;

    error_at_current(gen, gen->current.name);
  }

  return gen->current;
}

static void consume_expected(vaq_make_gen *gen, vaq_make_token_type type, const char *message) {
  if (!match(gen, type)) {
    error_at_current(gen, message);
  }
}

static bool check(vaq_make_gen *gen, vaq_make_token_type type) { return gen->current.type == type; }

static bool match(vaq_make_gen *gen, vaq_make_token_type type) {
  if (check(gen, type)) {
    consume(gen);
    return true;
  }
  return false;
}

static void error(vaq_make_gen *gen, const char *message) { error_at(gen, gen->previous, message); }

static void error_at(vaq_make_gen *gen, vaq_make_token token, const char *message) {
  if (gen->panic_mode)
    return;
  gen->panic_mode = true;
  fprintf(stderr, "[line %d] ERROR ", token.line + 1);
  if (token.type == TOKEN_EOF) {
    fprintf(stderr, "at end");
  } else if (token.type != TOKEN_ERROR) {
    fprintf(stderr, "at '%.*s'", token.name_length, token.name);
  }
  fprintf(stderr, ": %s\n", message);
  gen->had_error = true;
}

static void error_at_current(vaq_make_gen *gen, const char *message) {
  error_at(gen, gen->current, message);
}

void declaration(vaq_make_gen *gen) { statement(gen); }

void statement(vaq_make_gen *gen) {
  if (match(gen, TOKEN_PRINT)) {
    print_statement(gen);
  } else if (match(gen, TOKEN_INCLUDE)) {
    include_statement(gen);
  } else {
    expression_statement(gen);
  }

  if (gen->panic_mode) {
    synchronize(gen);
  }
}

void print_statement(vaq_make_gen *gen) {
  consume_expected(gen, TOKEN_LEFT_PAREN, "Expected '(' after 'print'.");
  vaq_make_value_print(grouping(gen));
  printf("\n");
  consume_expected(gen, TOKEN_SEMICOLON, "Expected ';' after print ')'.");
}

void include_statement(vaq_make_gen *gen) {
  vaq_make_value val = string(gen);
  // TODO: Implement
  consume_expected(gen, TOKEN_SEMICOLON, "Expected ';' after include string.");
}

void expression_statement(vaq_make_gen *gen) {
  expression(gen);
  consume_expected(gen, TOKEN_SEMICOLON, "Expected ';' after expression.");
}

vaq_make_value expression(vaq_make_gen *gen) { return assigment(gen); }

vaq_make_value assigment(vaq_make_gen *gen) {
  bool is_identifier = check(gen, TOKEN_IDENTIFIER);
  vaq_make_value val = equality(gen);

  if (match(gen, TOKEN_EQUAL)) {
    // If the left hand side is an identifier, then the pointer should be on the stack.
    vaq_make_value *val_ptr = gen->stack_size == 0 ? NULL : gen->stack[gen->stack_size - 1];
    vaq_make_value rhs = assigment(gen);
    if (is_identifier) {
      assign_variable(gen, val_ptr, rhs);
    } else {
      error(gen, "Invalid assignment target.");
    }
    return rhs;
  }

  return val;
}

vaq_make_value equality(vaq_make_gen *gen) {
  vaq_make_value lhs = comparison(gen);

  while (match(gen, TOKEN_EQUAL_EQUAL) || match(gen, TOKEN_NOT_EQUAL)) {
    vaq_make_token op = previous(gen);
    vaq_make_value rhs = comparison(gen);
    bool res = vaq_make_value_equals(lhs, rhs);
    if (op.type == TOKEN_NOT_EQUAL)
      res = !res;
    lhs = vaq_make_value_bool(res);
  }

  return lhs;
}

vaq_make_value comparison(vaq_make_gen *gen) {
  vaq_make_value lhs = term(gen);

  while (match(gen, TOKEN_LESS) || match(gen, TOKEN_LESS_EQUAL) || match(gen, TOKEN_GREATER) ||
         match(gen, TOKEN_GREATER_EQUAL)) {
    vaq_make_token op = previous(gen);
    vaq_make_value rhs = term(gen);
    if (lhs.type != VAL_NUMBER || rhs.type != VAL_NUMBER) {
      error(gen, "Expected numbers for comparison operation.");
    }
    int cmp = vaq_make_value_compare(lhs, rhs);
    bool res = false;
    switch (op.type) {
    case TOKEN_LESS:
      res = cmp < 0;
      break;
    case TOKEN_LESS_EQUAL:
      res = cmp <= 0;
      break;
    case TOKEN_GREATER:
      res = cmp > 0;
      break;
    case TOKEN_GREATER_EQUAL:
      res = cmp >= 0;
      break;
    default:
      break;
    }
    lhs = vaq_make_value_bool(res);
  }

  return lhs;
}

vaq_make_value term(vaq_make_gen *gen) {
  vaq_make_value lhs = factor(gen);

  while (match(gen, TOKEN_PLUS) || match(gen, TOKEN_MINUS)) {
    vaq_make_token op = previous(gen);
    vaq_make_value rhs = factor(gen);

    bool both_numbers = lhs.type == VAL_NUMBER && rhs.type == VAL_NUMBER;
    bool both_str = lhs.type == VAL_OBJ && rhs.type == VAL_OBJ && lhs.as.obj->type == OBJ_STRING &&
                    rhs.as.obj->type == OBJ_STRING;
    if (op.type == TOKEN_PLUS) {
      if (both_numbers) {
        lhs = vaq_make_value_number(lhs.as.number + rhs.as.number);
      } else if (both_str) {
        vaq_make_obj_string *lhs_str = (vaq_make_obj_string *)lhs.as.obj;
        vaq_make_obj_string *rhs_str = (vaq_make_obj_string *)rhs.as.obj;
        int buf_len = lhs_str->length + rhs_str->length;
        char *buf = malloc(sizeof(char) * (buf_len + 1));
        memcpy(buf, lhs_str->chars, lhs_str->length);
        memcpy(buf + lhs_str->length, rhs_str->chars, rhs_str->length);
        buf[buf_len] = '\0';
        vaq_make_obj_string *result = vaq_make_obj_string_new(buf, buf_len, false);
        return vaq_make_value_obj((vaq_make_obj *)result);
      } else {
        error(gen, "Expected numbers or strings for addition.");
      }
    } else if (op.type == TOKEN_MINUS) {
      if (both_numbers) {
        lhs = vaq_make_value_number(lhs.as.number - rhs.as.number);
      } else {
        error(gen, "Expected numbers for subtraction.");
      }
    }
  }

  return lhs;
}

vaq_make_value factor(vaq_make_gen *gen) {
  vaq_make_value lhs = unary(gen);

  while (match(gen, TOKEN_STAR) || match(gen, TOKEN_SLASH)) {
    vaq_make_token op = previous(gen);
    vaq_make_value rhs = unary(gen);

    if (lhs.type == VAL_NUMBER && rhs.type == VAL_NUMBER) {
      if (op.type == TOKEN_STAR) {
        lhs = vaq_make_value_number(lhs.as.number * rhs.as.number);
      } else if (op.type == TOKEN_SLASH) {
        lhs = vaq_make_value_number(lhs.as.number / rhs.as.number);
      }
    } else {
      error(gen, "Expected numbers for multiplication or division.");
    }
  }

  return lhs;
}

vaq_make_value unary(vaq_make_gen *gen) {
  if (match(gen, TOKEN_NOT)) {
    vaq_make_value val = unary(gen);
    if (val.type == VAL_BOOL) {
      return vaq_make_value_bool(!val.as.boolean);
    } else {
      error(gen, "Expected boolean for logical not.");
    }
  } else if (match(gen, TOKEN_MINUS)) {
    vaq_make_value val = unary(gen);
    if (val.type == VAL_NUMBER) {
      return vaq_make_value_number(-val.as.number);
    } else {
      error(gen, "Expected number for unary minus.");
    }
  }

  return call(gen);
}

vaq_make_value call(vaq_make_gen *gen) {
  vaq_make_value val = primary(gen);

  // TODO: Implement this for functions and property access
  //
  while (true) {
    if (match(gen, TOKEN_LEFT_PAREN)) {
      vaq_make_value_array args = arguments(gen);
      consume_expected(gen, TOKEN_RIGHT_PAREN, "Expected ')' after argument list");
      val = call_value(gen, val, args);
      vaq_make_value_array_free(&args);
    }
    // else if (match(gen, TOKEN_DOT)) {
    // }
  }

  return val;
}

vaq_make_value primary(vaq_make_gen *gen) {
  if (match(gen, TOKEN_FALSE)) {
    return vaq_make_value_bool(false);
  } else if (match(gen, TOKEN_TRUE)) {
    return vaq_make_value_bool(true);
  } else if (match(gen, TOKEN_NULL)) {
    return vaq_make_value_nil();
  } else if (match(gen, TOKEN_NUMBER)) {
    return number(gen);
  } else if (match(gen, TOKEN_STRING)) {
    return string(gen);
  } else if (match(gen, TOKEN_LEFT_PAREN)) {
    return grouping(gen);
  } else if (match(gen, TOKEN_IDENTIFIER)) {
    return identifier(gen);
  }

  error(gen, "Expected expression.");
  return vaq_make_value_nil();
}

vaq_make_value_array arguments(vaq_make_gen *gen) {
  vaq_make_value_array arr;
  vaq_make_value_array_new(&arr);
  do {
    vaq_make_value_array_push(&arr, assigment(gen));
  } while (match(gen, TOKEN_COMMA));
  return arr;
}

vaq_make_value grouping(vaq_make_gen *gen) {
  vaq_make_value val = expression(gen);
  consume_expected(gen, TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
  return val;
}

vaq_make_value number(vaq_make_gen *gen) { return vaq_make_value_number(atof(previous(gen).name)); }

vaq_make_value string(vaq_make_gen *gen) {
  // TODO: Implement
  vaq_make_token token = previous(gen);
  vaq_make_obj_string *obj = vaq_make_obj_string_new((char *)token.name, token.name_length, true);
  return vaq_make_value_obj((vaq_make_obj *)obj);
}

vaq_make_value identifier(vaq_make_gen *gen) {
  vaq_make_value *res = resolve_variable(gen, previous(gen));
  if (res)
    return *res;
  return vaq_make_value_nil();
}

vaq_make_value assign_variable(vaq_make_gen *gen, vaq_make_value *lhs, vaq_make_value rhs) {
  // lhs->type = rhs.type;
  // lhs->as = rhs.as;
  *lhs = rhs;
  return *lhs;
}

vaq_make_value call_value(vaq_make_gen *gen, vaq_make_value val, vaq_make_value_array args) {
  error(gen, "Operation is not yet implemented.");
  return vaq_make_value_nil();
}

vaq_make_value *resolve_variable(vaq_make_gen *gen, vaq_make_token name) {
  // If variable exists, return it
  for (int i = 0; i < gen->variables.size; i++) {
    vaq_make_variable *variable = &gen->variables.values[i];
    if (variable->name.name_length == name.name_length &&
        strncmp(variable->name.name, name.name, name.name_length) == 0) {
      return &variable->value;
    }
  }

  // If variable doesn't exist, create it
  vaq_make_variable variable;
  variable.name = name;
  variable.value = vaq_make_value_nil();
  vaq_make_variable_array_push(&gen->variables, variable);

  // Return the copy of our variable from the heap.
  vaq_make_value *ptr = &gen->variables.values[gen->variables.size - 1].value;
  // Push the pointer onto the stack.
  gen->stack[gen->stack_size++] = ptr;

  return ptr;
}
