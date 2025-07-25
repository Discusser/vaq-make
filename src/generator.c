#include "generator.h"
#include "array.h"
#include "generator-priv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void synchronize(vmake_gen *gen);
static vmake_token previous(vmake_gen *gen);
static vmake_token consume(vmake_gen *gen);
static void consume_expected(vmake_gen *gen, vmake_token_type type, const char *message);
static bool check(vmake_gen *gen, vmake_token_type type);
static bool match(vmake_gen *gen, vmake_token_type type);

static void error(vmake_gen *gen, const char *message);
static void error_at(vmake_gen *gen, vmake_token token, const char *message);
static void error_at_current(vmake_gen *gen, const char *message);

bool vmake_generate_build(vmake_scanner *scanner, const char *file_path) {
  vmake_gen gen;
  gen.file_name = strrchr(file_path, '/');
  if (gen.file_name == NULL)
    gen.file_name = file_path;
  else
    gen.file_name++;
  gen.scanner = scanner;
  gen.had_error = false;
  gen.panic_mode = false;
  gen.stack_size = 0;

  // TODO: Store a hash table of native functions in vmake_gen. This table is populated in
  // vmake_generate_build. Because vmake_generate_build is currently called for each file, and
  // multi-file support is planned, it might be wiser to make the vmake_gen struct encapsulate
  // multiple files. One way to do this is by having an `enclosing` field in the struct, which
  // points to the parent vmake_gen.
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

  vmake_variable_array_new(&gen.variables);

  consume(&gen);
  while (!check(&gen, TOKEN_EOF)) {
    declaration(&gen);
  }
  consume_expected(&gen, TOKEN_EOF, "Expected end of expression.");

  return !gen.had_error;
}

static void synchronize(vmake_gen *gen) {
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

static vmake_token previous(vmake_gen *gen) { return gen->previous; }

static vmake_token consume(vmake_gen *gen) {
  gen->previous = gen->current;

  while (true) {
    gen->current = vmake_scan_token(gen->scanner);
    if (gen->current.type != TOKEN_ERROR)
      break;

    error_at_current(gen, gen->current.name);
  }

  return gen->current;
}

static void consume_expected(vmake_gen *gen, vmake_token_type type, const char *message) {
  if (!match(gen, type)) {
    error_at_current(gen, message);
  }
}

static bool check(vmake_gen *gen, vmake_token_type type) { return gen->current.type == type; }

static bool match(vmake_gen *gen, vmake_token_type type) {
  if (check(gen, type)) {
    consume(gen);
    return true;
  }
  return false;
}

static void error(vmake_gen *gen, const char *message) { error_at(gen, gen->previous, message); }

static void error_at(vmake_gen *gen, vmake_token token, const char *message) {
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

static void error_at_current(vmake_gen *gen, const char *message) {
  error_at(gen, gen->current, message);
}

void declaration(vmake_gen *gen) { statement(gen); }

void statement(vmake_gen *gen) {
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

void print_statement(vmake_gen *gen) {
  consume_expected(gen, TOKEN_LEFT_PAREN, "Expected '(' after 'print'.");
  vmake_value_print(grouping(gen));
  printf("\n");
  consume_expected(gen, TOKEN_SEMICOLON, "Expected ';' after print ')'.");
}

void include_statement(vmake_gen *gen) {
  vmake_value val = string(gen);
  // TODO: Implement
  consume_expected(gen, TOKEN_SEMICOLON, "Expected ';' after include string.");
}

void expression_statement(vmake_gen *gen) {
  expression(gen);
  consume_expected(gen, TOKEN_SEMICOLON, "Expected ';' after expression.");
}

vmake_value expression(vmake_gen *gen) { return assigment(gen); }

vmake_value assigment(vmake_gen *gen) {
  bool is_identifier = check(gen, TOKEN_IDENTIFIER);
  vmake_value val = equality(gen);

  if (match(gen, TOKEN_EQUAL)) {
    // If the left hand side is an identifier, then the pointer should be on the stack.
    vmake_value *val_ptr = gen->stack_size == 0 ? NULL : gen->stack[gen->stack_size - 1];
    vmake_value rhs = assigment(gen);
    if (is_identifier) {
      assign_variable(gen, val_ptr, rhs);
    } else {
      error(gen, "Invalid assignment target.");
    }
    return rhs;
  }

  return val;
}

vmake_value equality(vmake_gen *gen) {
  vmake_value lhs = comparison(gen);

  while (match(gen, TOKEN_EQUAL_EQUAL) || match(gen, TOKEN_NOT_EQUAL)) {
    vmake_token op = previous(gen);
    vmake_value rhs = comparison(gen);
    bool res = vmake_value_equals(lhs, rhs);
    if (op.type == TOKEN_NOT_EQUAL)
      res = !res;
    lhs = vmake_value_bool(res);
  }

  return lhs;
}

vmake_value comparison(vmake_gen *gen) {
  vmake_value lhs = term(gen);

  while (match(gen, TOKEN_LESS) || match(gen, TOKEN_LESS_EQUAL) || match(gen, TOKEN_GREATER) ||
         match(gen, TOKEN_GREATER_EQUAL)) {
    vmake_token op = previous(gen);
    vmake_value rhs = term(gen);
    if (lhs.type != VAL_NUMBER || rhs.type != VAL_NUMBER) {
      error(gen, "Expected numbers for comparison operation.");
    }
    int cmp = vmake_value_compare(lhs, rhs);
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
    lhs = vmake_value_bool(res);
  }

  return lhs;
}

vmake_value term(vmake_gen *gen) {
  vmake_value lhs = factor(gen);

  while (match(gen, TOKEN_PLUS) || match(gen, TOKEN_MINUS)) {
    vmake_token op = previous(gen);
    vmake_value rhs = factor(gen);

    bool both_numbers = lhs.type == VAL_NUMBER && rhs.type == VAL_NUMBER;
    bool both_str = lhs.type == VAL_OBJ && rhs.type == VAL_OBJ && lhs.as.obj->type == OBJ_STRING &&
                    rhs.as.obj->type == OBJ_STRING;
    if (op.type == TOKEN_PLUS) {
      if (both_numbers) {
        lhs = vmake_value_number(lhs.as.number + rhs.as.number);
      } else if (both_str) {
        vmake_obj_string *lhs_str = (vmake_obj_string *)lhs.as.obj;
        vmake_obj_string *rhs_str = (vmake_obj_string *)rhs.as.obj;
        int buf_len = lhs_str->length + rhs_str->length;
        char *buf = malloc(sizeof(char) * (buf_len + 1));
        memcpy(buf, lhs_str->chars, lhs_str->length);
        memcpy(buf + lhs_str->length, rhs_str->chars, rhs_str->length);
        buf[buf_len] = '\0';
        vmake_obj_string *result = vmake_obj_string_new(buf, buf_len, false);
        return vmake_value_obj((vmake_obj *)result);
      } else {
        error(gen, "Expected numbers or strings for addition.");
      }
    } else if (op.type == TOKEN_MINUS) {
      if (both_numbers) {
        lhs = vmake_value_number(lhs.as.number - rhs.as.number);
      } else {
        error(gen, "Expected numbers for subtraction.");
      }
    }
  }

  return lhs;
}

vmake_value factor(vmake_gen *gen) {
  vmake_value lhs = unary(gen);

  while (match(gen, TOKEN_STAR) || match(gen, TOKEN_SLASH)) {
    vmake_token op = previous(gen);
    vmake_value rhs = unary(gen);

    if (lhs.type == VAL_NUMBER && rhs.type == VAL_NUMBER) {
      if (op.type == TOKEN_STAR) {
        lhs = vmake_value_number(lhs.as.number * rhs.as.number);
      } else if (op.type == TOKEN_SLASH) {
        lhs = vmake_value_number(lhs.as.number / rhs.as.number);
      }
    } else {
      error(gen, "Expected numbers for multiplication or division.");
    }
  }

  return lhs;
}

vmake_value unary(vmake_gen *gen) {
  if (match(gen, TOKEN_NOT)) {
    vmake_value val = unary(gen);
    if (val.type == VAL_BOOL) {
      return vmake_value_bool(!val.as.boolean);
    } else {
      error(gen, "Expected boolean for logical not.");
    }
  } else if (match(gen, TOKEN_MINUS)) {
    vmake_value val = unary(gen);
    if (val.type == VAL_NUMBER) {
      return vmake_value_number(-val.as.number);
    } else {
      error(gen, "Expected number for unary minus.");
    }
  }

  return call(gen);
}

vmake_value call(vmake_gen *gen) {
  vmake_value val = primary(gen);

  // TODO: Implement this for functions and property access
  //
  while (true) {
    if (match(gen, TOKEN_LEFT_PAREN)) {
      vmake_value_array args = arguments(gen);
      consume_expected(gen, TOKEN_RIGHT_PAREN, "Expected ')' after argument list");
      val = call_value(gen, val, args);
      vmake_value_array_free(&args);
    }
    // else if (match(gen, TOKEN_DOT)) {
    // }
  }

  return val;
}

vmake_value primary(vmake_gen *gen) {
  if (match(gen, TOKEN_FALSE)) {
    return vmake_value_bool(false);
  } else if (match(gen, TOKEN_TRUE)) {
    return vmake_value_bool(true);
  } else if (match(gen, TOKEN_NULL)) {
    return vmake_value_nil();
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
  return vmake_value_nil();
}

vmake_value_array arguments(vmake_gen *gen) {
  vmake_value_array arr;
  vmake_value_array_new(&arr);
  do {
    vmake_value_array_push(&arr, assigment(gen));
  } while (match(gen, TOKEN_COMMA));
  return arr;
}

vmake_value grouping(vmake_gen *gen) {
  vmake_value val = expression(gen);
  consume_expected(gen, TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
  return val;
}

vmake_value number(vmake_gen *gen) { return vmake_value_number(atof(previous(gen).name)); }

vmake_value string(vmake_gen *gen) {
  // TODO: Implement
  vmake_token token = previous(gen);
  vmake_obj_string *obj = vmake_obj_string_new((char *)token.name, token.name_length, true);
  return vmake_value_obj((vmake_obj *)obj);
}

vmake_value identifier(vmake_gen *gen) {
  vmake_value *res = resolve_variable(gen, previous(gen));
  if (res)
    return *res;
  return vmake_value_nil();
}

vmake_value assign_variable(vmake_gen *gen, vmake_value *lhs, vmake_value rhs) {
  // lhs->type = rhs.type;
  // lhs->as = rhs.as;
  *lhs = rhs;
  return *lhs;
}

vmake_value call_value(vmake_gen *gen, vmake_value val, vmake_value_array args) {
  error(gen, "Operation is not yet implemented.");
  return vmake_value_nil();
}

vmake_value *resolve_variable(vmake_gen *gen, vmake_token name) {
  // If variable exists, return it
  for (int i = 0; i < gen->variables.size; i++) {
    vmake_variable *variable = &gen->variables.values[i];
    if (variable->name.name_length == name.name_length &&
        strncmp(variable->name.name, name.name, name.name_length) == 0) {
      return &variable->value;
    }
  }

  // If variable doesn't exist, create it
  vmake_variable variable;
  variable.name = name;
  variable.value = vmake_value_nil();
  vmake_variable_array_push(&gen->variables, variable);

  // Return the copy of our variable from the heap.
  vmake_value *ptr = &gen->variables.values[gen->variables.size - 1].value;
  // Push the pointer onto the stack.
  gen->stack[gen->stack_size++] = ptr;

  return ptr;
}
