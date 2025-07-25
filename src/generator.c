#include "generator.h"
#include "array.h"
#include "common.h"
#include "file.h"
#include "generator-priv.h"
#include "native.h"
#include <math.h>
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

static vmake_value *pop(vmake_gen *gen);

bool vmake_generate_build(vmake_scanner *scanner, vmake_state *state, const char *file_path) {
  vmake_gen gen;
  gen.state = state;
  gen.file_path = file_path;

  vmake_value path_key = vmake_value_obj(
      (vmake_obj *)vmake_obj_string_new(gen.state, (char *)file_path, strlen(file_path), true));
  if (vmake_value_array_contains(&gen.state->include_stack, path_key)) {
    error_at(&gen, (vmake_token){.type = TOKEN_NONE}, "Cyclic include detected");
    for (int i = gen.state->include_stack.size - 1; i >= 0; i--) {
      printf("  included from '%s'\n",
             ((vmake_obj_string *)gen.state->include_stack.values[i].as.obj)->chars);
    }
    exit(1);
  }
  vmake_value_array_push(&gen.state->include_stack, path_key);

  gen.scanner = scanner;
  gen.previous.type = TOKEN_NONE;
  gen.current.type = TOKEN_NONE;
  gen.stack_size = 0;
  gen.scope_depth = 0;

  vmake_variable_array_new(&gen.locals);

  vmake_define_natives(gen.state);

  consume(&gen);
  while (!check(&gen, TOKEN_EOF)) {
    declaration(&gen);
  }
  consume_expected(&gen, TOKEN_EOF, "Expected end of expression.");

  vmake_value_array_pop(&gen.state->include_stack);

  return !gen.state->had_error;
}

static void synchronize(vmake_gen *gen) {
  gen->state->panic_mode = false;

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
  if (gen->state->panic_mode)
    return;
  gen->state->panic_mode = true;
  if (token.type == TOKEN_NONE) {
    fprintf(stderr, "[%s] ERROR", gen->file_path);
  } else {
    fprintf(stderr, "[%s:%d] ERROR ", gen->file_path, token.line + 1);
    if (token.type == TOKEN_EOF) {
      fprintf(stderr, "at end");
    } else if (token.type != TOKEN_ERROR) {
      fprintf(stderr, "at '%.*s'", token.name_length, token.name);
    }
  }
  fprintf(stderr, ": %s\n", message);
  gen->state->had_error = true;
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

  if (gen->state->panic_mode) {
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
  vmake_value val = expression(gen);
  if (!vmake_value_is_string(val)) {
    error(gen, "Expected string after 'include'");
    return;
  }

  char *include_path = ((vmake_obj_string *)val.as.obj)->chars;
  printf("Including '%s'\n", include_path);
  char *new_path = vmake_path_rel(gen->file_path, include_path);
  if (new_path == NULL) {
    char *str;
    asprintf(&str, "No file with path '%s' was found", include_path);
    error(gen, str);
    free(str);
    return;
  }
  vmake_process_path(gen->state, new_path);
  free(new_path);
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
  vmake_token prev = previous(gen);

  if (match(gen, TOKEN_EQUAL)) {
    // If the left hand side is an identifier, then the pointer should be on the stack.
    vmake_value *val_ptr = resolve_variable(gen, prev);
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
        vmake_obj_string *result = vmake_obj_string_new(gen->state, buf, buf_len, false);
        lhs = vmake_value_obj((vmake_obj *)result);
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

  return subscript(gen);
}

vmake_value subscript(vmake_gen *gen) {
  vmake_value val = call(gen);
  vmake_token prev = previous(gen);

  while (match(gen, TOKEN_LEFT_SQUARE_BRACKET)) {
    vmake_value *target = resolve_variable(gen, prev);

    if (target == NULL) {
      error(gen, "Cannot index into null value.");
      break;
    }

    vmake_value index = expression(gen);
    if (index.type != VAL_NUMBER) {
      char *str;
      asprintf(&str, "Expected number for array subscript, found '%s' instead.",
               vmake_value_to_string(index));
      error(gen, str);
      free(str);
      break;
    }

    size_t index_trunc = trunc(index.as.number);
    if (index_trunc != index.as.number || fabs(index.as.number) > SIZE_MAX) {
      char *str;
      asprintf(&str, "Invalid number for array subscript %s.", vmake_value_to_string(index));
      error(gen, str);
      free(str);
      break;
    }

    if (!vmake_value_is_array(*target)) {
      char *str;
      asprintf(&str, "Expected array as subscript target, found '%s' instead.",
               vmake_value_to_string(*target));
      error(gen, str);
      free(str);
      break;
    }

    vmake_obj_array *arr = (vmake_obj_array *)val.as.obj;
    if (index_trunc >= arr->array->size) {
      char *str;
      asprintf(&str, "Array subscript index %zu is too big for array of size %i.", index_trunc,
               arr->array->size);
      error(gen, str);
      free(str);
      break;
    }

    // Once we've checked all the possibles cases, we can finally just index into the array.
    val = arr->array->values[index_trunc];

    consume_expected(gen, TOKEN_RIGHT_SQUARE_BRACKET, "Expected ']' after array subscript.");
  }

  return val;
}

vmake_value call(vmake_gen *gen) {
  vmake_value val = primary(gen);

  // TODO: Implement this for property access
  while (true) {
    if (match(gen, TOKEN_LEFT_PAREN)) {
      vmake_value_array args = arguments(gen);
      consume_expected(gen, TOKEN_RIGHT_PAREN, "Expected ')' after argument list");
      if (vmake_value_is_native(val)) {
        vmake_obj_native *native = (vmake_obj_native *)val.as.obj;
        if (native->arity != args.size) {
          char *str;
          char *native_name = vmake_obj_to_string((vmake_obj *)native);
          asprintf(&str, "Expected %i arguments for native function '%s' but found %i instead.",
                   native->arity, native_name, args.size);
          free(native_name);
          error(gen, str);
          free(str);
        }
      }
      val = call_value(gen, val, args);
      vmake_value_array_free(&args);
    } else {
      break;
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
  } else if (match(gen, TOKEN_LEFT_SQUARE_BRACKET)) {
    return array(gen);
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
  // TODO: Implement kwargs. That means that this function should return a vmake_arguments struct
  // defined like so:
  //
  // struct vmake_arguments {
  //  vmake_value_array args;
  //  vmake_table kwargs;
  // }
  //
  // kwargs must imperatively come after the last positional argument. Reading a positional argument
  // after a keyword argument is an error.
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

vmake_value array(vmake_gen *gen) {
  vmake_value_array arr;
  vmake_value_array_new(&arr);
  do {
    vmake_value_array_push(&arr, assigment(gen));
  } while (match(gen, TOKEN_COMMA));
  consume_expected(gen, TOKEN_RIGHT_SQUARE_BRACKET, "Expected ']' after array.");
  return vmake_value_obj((vmake_obj *)vmake_obj_array_new(gen->state, arr));
}

vmake_value number(vmake_gen *gen) { return vmake_value_number(atof(previous(gen).name)); }

vmake_value string(vmake_gen *gen) {
  vmake_token token = previous(gen);
  vmake_obj_string *obj =
      vmake_obj_string_new(gen->state, (char *)token.name, token.name_length, true);
  return vmake_value_obj((vmake_obj *)obj);
}

vmake_value identifier(vmake_gen *gen) {
  vmake_token name = previous(gen);
  vmake_value *res = NULL;
  // If we're in a global scope, we first look for a global variable, and create it if it doesn't
  // exist.
  if (gen->scope_depth == 0) {
    res = resolve_global(gen, name);
    if (!res)
      res = create_global(gen, name);
  }

  // If we're inside a local scope, we look for a local variable first, then a global variable, and
  // finally we create a local variable if nothing was found
  if (gen->scope_depth > 0) {
    res = resolve_local(gen, name);
    if (!res) {
      res = resolve_global(gen, name);
    }
    if (!res) {
      res = create_local(gen, name);
    }
  }

  gen->stack[gen->stack_size++] = res;
  return *res;
}

vmake_value assign_variable(vmake_gen *gen, vmake_value *lhs, vmake_value rhs) {
  // lhs->type = rhs.type;
  // lhs->as = rhs.as;
  *lhs = rhs;
  return *lhs;
}

vmake_value call_value(vmake_gen *gen, vmake_value val, vmake_value_array args) {
  switch (val.as.obj->type) {
  case OBJ_NATIVE: {
    vmake_obj_native *native = (vmake_obj_native *)val.as.obj;
    return native->function(args.size, args.values);
  }
  default:
    error(gen, "Object is not callable.");
    break;
  }
  return vmake_value_nil();
}

vmake_value *resolve_variable(vmake_gen *gen, vmake_token name) {
  if (gen->scope_depth == 0)
    return resolve_global(gen, name);

  return resolve_local(gen, name);
}

vmake_value *resolve_local(vmake_gen *gen, vmake_token name) {
  if (gen->locals.size == 0)
    return NULL;

  // If variable exists, return it
  for (int i = gen->locals.size; i >= 0; i--) {
    vmake_variable *variable = &gen->locals.values[i];
    if (variable->name.name_length == name.name_length &&
        strncmp(variable->name.name, name.name, name.name_length) == 0) {
      vmake_value *ptr = &variable->value;
      return ptr;
    }
  }

  return NULL;
}

vmake_value *resolve_global(vmake_gen *gen, vmake_token name) {
  vmake_obj_string *str =
      vmake_obj_string_new(gen->state, (char *)name.name, name.name_length, true);
  vmake_value key = vmake_value_obj((vmake_obj *)str);
  vmake_value *value = NULL;

  // Try to retrieve the value, or create it if it doesn't exist
  vmake_table_get(&gen->state->globals, key, &value);

  return value;
}

vmake_value *create_local(vmake_gen *gen, vmake_token name) {
  vmake_variable variable;
  variable.name = name;
  variable.value = vmake_value_nil();
  vmake_variable_array_push(&gen->locals, variable);

  vmake_value *ptr = &gen->locals.values[gen->locals.size - 1].value;

  return ptr;
}

vmake_value *create_global(vmake_gen *gen, vmake_token name) {
  vmake_obj_string *str =
      vmake_obj_string_new(gen->state, (char *)name.name, name.name_length, true);
  vmake_value key = vmake_value_obj((vmake_obj *)str);

  vmake_value value = vmake_value_nil();
  vmake_value *inserted;
  vmake_table_put_ret(&gen->state->globals, key, &value, &inserted);

  return inserted;
}

static vmake_value *pop(vmake_gen *gen) {
  return gen->stack_size == 0 ? (error(gen, "Tried to pop from empty stack."), NULL)
                              : gen->stack[gen->stack_size--];
}
