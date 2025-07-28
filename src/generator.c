#include "generator.h"
#include "array.h"
#include "common.h"
#include "file.h"
#include "generator-priv.h"
#include "native/class.h"
#include "native/fun.h"
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

static void error(vmake_gen *gen, vmake_error_context ctx, const char *message);
static void error_at(vmake_gen *gen, vmake_token token, vmake_error_context ctx,
                     const char *message);
static void error_at_current(vmake_gen *gen, vmake_error_context ctx, const char *message);

static vmake_value *pop(vmake_gen *gen);
static void push(vmake_gen *gen, vmake_value *val);
static void push_obj(vmake_gen *gen, vmake_obj *obj);
// Equivalent to pop(gen) followed by push(gen, val);
static void modify_top(vmake_gen *gen, vmake_value *val);

bool vmake_generate_build(vmake_scanner *scanner, vmake_state *state, const char *file_path) {
  vmake_gen gen;
  gen.state = state;
  gen.file_path = file_path;
  gen.scanner = scanner;
  gen.previous.type = TOKEN_NONE;
  gen.current.type = TOKEN_NONE;
  gen.stack_size = 0;
  gen.scope_depth = 0;

  vmake_variable_array_new(&gen.locals);

  vmake_define_native_classes(gen.state);
  vmake_define_native_functions(gen.state);

  consume(&gen);
  while (!check(&gen, TOKEN_EOF)) {
    declaration(&gen);
  }
  consume_expected(&gen, TOKEN_EOF, "Expected end of expression.");

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

    error_at_current(gen, CTX_SYNTAX, gen->current.name);
  }

  return gen->current;
}

static void consume_expected(vmake_gen *gen, vmake_token_type type, const char *message) {
  if (!match(gen, type)) {
    error_at_current(gen, CTX_SYNTAX, message);
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

static void error(vmake_gen *gen, vmake_error_context ctx, const char *message) {
  error_at(gen, gen->previous, ctx, message);
}

static void error_at(vmake_gen *gen, vmake_token token, vmake_error_context ctx,
                     const char *message) {
  vmake_error_exit(gen, CTX_USER, &token, message);
}

static void error_at_current(vmake_gen *gen, vmake_error_context ctx, const char *message) {
  error_at(gen, gen->current, ctx, message);
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
  pop(gen);
}

void include_statement(vmake_gen *gen) {
  vmake_value val = expression(gen);
  pop(gen);
  if (!vmake_value_is_string(val)) {
    error(gen, CTX_SYNTAX, "Expected string after 'include'");
    return;
  }

  // The include path is either absolute, or relative to the current path
  char *include_path = ((vmake_obj_string *)val.as.obj)->chars;
  // First we resolve the relative path
  char *resolved_path = vmake_path_rel(gen->file_path, include_path);
  if (resolved_path == NULL) {
    char *str;
    asprintf(&str, "No file with path '%s' was found", include_path);
    error(gen, CTX_INTERNAL, str);
    free(str);
    return;
  }

  // Then we convert the relative path to an absolute path in order to store it in the include stack
  char *abs_path = vmake_path_abs(resolved_path);
  vmake_value key = vmake_value_obj((vmake_obj *)vmake_obj_string_const(gen->state, abs_path));
  free(resolved_path);
  if (vmake_value_array_contains(&gen->state->include_stack, key)) {
    char *val_str = vmake_value_to_string(val);
    char *str;
    asprintf(&str, "Cyclic include detected while including %s", val_str);
    error(gen, CTX_USER, str);
    free(str);
    free(val_str);
    exit(1);
  }

  vmake_value_array_push(&gen->state->include_stack, key);
  vmake_process_path(gen->state, abs_path);
  vmake_value_array_pop(&gen->state->include_stack);

  free(abs_path);
  consume_expected(gen, TOKEN_SEMICOLON, "Expected ';' after include string.");
}

void expression_statement(vmake_gen *gen) {
  expression(gen);
  consume_expected(gen, TOKEN_SEMICOLON, "Expected ';' after expression.");
  pop(gen);
}

vmake_value expression(vmake_gen *gen) {
  int before = gen->stack_size;
  vmake_value res = assignment(gen);
  // An expression always produces a result. With our recursive-descent parser, we return that
  // result to the calling function, but sometimes we want to return a pointer to the result (for
  // variables), and we can do that via the stack. If there was no pointer pushed to the stack, we
  // push NULL so that statements don't pop too many variables from the stack. This is definitely
  // not the best way to handle this type of problem, but it's a limitation of the fact that we
  // perform everything in a single pass.
  if (gen->stack_size == before) {
    push(gen, NULL);
  }
  return res;
}

vmake_value assignment(vmake_gen *gen) {
  bool is_identifier = check(gen, TOKEN_IDENTIFIER);
  vmake_value val = equality(gen);
  vmake_token prev = previous(gen);

  if (match(gen, TOKEN_EQUAL)) {
    // If the left hand side is an identifier, then the pointer should be on the stack.
    // vmake_value *val_ptr = resolve_variable(gen, prev);
    vmake_value *val_ptr = pop(gen);
    vmake_value rhs = assignment(gen);
    if (is_identifier) {
      assign_variable(gen, val_ptr, rhs);
    } else {
      error(gen, CTX_USER, "Invalid assignment target.");
    }
    push(gen, val_ptr);
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
      error(gen, CTX_USER, "Expected numbers for comparison operation.");
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
        error(gen, CTX_USER, "Expected numbers or strings for addition.");
      }
    } else if (op.type == TOKEN_MINUS) {
      if (both_numbers) {
        lhs = vmake_value_number(lhs.as.number - rhs.as.number);
      } else {
        error(gen, CTX_USER, "Expected numbers for subtraction.");
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
      error(gen, CTX_USER, "Expected numbers for multiplication or division.");
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
      error(gen, CTX_USER, "Expected boolean for logical not.");
    }
  } else if (match(gen, TOKEN_MINUS)) {
    vmake_value val = unary(gen);
    if (val.type == VAL_NUMBER) {
      return vmake_value_number(-val.as.number);
    } else {
      error(gen, CTX_USER, "Expected number for unary minus.");
    }
  }

  return subscript(gen);
}

vmake_value subscript(vmake_gen *gen) {
  vmake_value val = call(gen);
  vmake_token prev = previous(gen);

  while (match(gen, TOKEN_LEFT_SQUARE_BRACKET)) {
    vmake_value *target = pop(gen);
    // vmake_value *target = resolve_variable(gen, prev);

    if (target == NULL) {
      error(gen, CTX_USER, "Cannot index into null value.");
      break;
    }

    vmake_value index = expression(gen);
    if (index.type != VAL_NUMBER) {
      char *str;
      asprintf(&str, "Expected number for array subscript, found %s instead.",
               vmake_value_to_string(index));
      error(gen, CTX_USER, str);
      free(str);
      break;
    }

    size_t index_trunc = trunc(index.as.number);
    if (index_trunc != index.as.number || fabs(index.as.number) > SIZE_MAX) {
      char *str;
      asprintf(&str, "Invalid number for array subscript %s.", vmake_value_to_string(index));
      error(gen, CTX_USER, str);
      free(str);
      break;
    }

    if (!vmake_value_is_array(*target)) {
      char *str;
      asprintf(&str, "Expected array as subscript target, found %s instead.",
               vmake_value_to_string(*target));
      error(gen, CTX_USER, str);
      free(str);
      break;
    }

    vmake_obj_array *arr = (vmake_obj_array *)val.as.obj;
    if (index_trunc >= arr->array->size) {
      char *str;
      asprintf(&str, "Array subscript index %zu is too big for array of size %i.", index_trunc,
               arr->array->size);
      error(gen, CTX_USER, str);
      free(str);
      break;
    }

    // Once we've checked all the possibles cases, we can finally just index into the array.
    val = arr->array->values[index_trunc];
    push(gen, arr->array->values + index_trunc);

    consume_expected(gen, TOKEN_RIGHT_SQUARE_BRACKET, "Expected ']' after array subscript.");

    // if (match(gen, TOKEN_EQUAL)) {
    //   vmake_value rhs = assignment(gen);
    //   assign_variable(gen, arr->array->values + index_trunc, rhs);
    // } else {
    // }
  }

  return val;
}

vmake_value call(vmake_gen *gen) {
  vmake_value val = primary(gen);

  while (true) {
    vmake_token prev = previous(gen);
    if (match(gen, TOKEN_LEFT_PAREN)) {
      vmake_arguments args = arguments(gen);
      consume_expected(gen, TOKEN_RIGHT_PAREN, "Expected ')' after argument list");

      switch (val.as.obj->type) {
      case OBJ_NATIVE:
        val = call_native(gen, val, &args);
        break;
      default:
        error_at(gen, prev, CTX_USER, "Object is not callable.");
      }

      vmake_value_array_free(&args.args);
      vmake_table_free(&args.kwargs);
    } else if (match(gen, TOKEN_DOT)) {
      consume_expected(gen, TOKEN_IDENTIFIER, "Expected property name after '.'.");
      vmake_value prop = identifier_string(gen);
      if (!vmake_value_is_instance(val)) {
        char *buf;
        asprintf(&buf, "Expected instance for property access, but found %s instead.",
                 val.type == VAL_OBJ ? vmake_obj_type_to_string(val.as.obj->type)
                                     : vmake_value_type_to_string(val.type));
        error_at(gen, prev, CTX_USER, buf);
        free(buf);
        break;
      }
      vmake_obj_instance *inst = (vmake_obj_instance *)val.as.obj;
      vmake_value *tmp = NULL;
      vmake_table_get(&inst->fields, prop, &tmp);
      if (tmp == NULL) {
        vmake_table_get(&inst->klass->methods, prop, &tmp);
        if (tmp == NULL) {
          char *buf;
          char *prop_str = vmake_value_to_string(prop);
          char *class_str = vmake_obj_to_string((vmake_obj *)inst->klass->name);
          asprintf(&buf, "Invalid property %s on instance of %s.", prop_str, class_str);
          error_at(gen, prev, CTX_USER, buf);
          free(class_str);
          free(prop_str);
          free(buf);
          break;
        }

        // Disallow storing methods as variables so we don't have to deal with closures and stuff
        // like that.
        if (!match(gen, TOKEN_LEFT_PAREN)) {
          error(gen, CTX_SYNTAX, "Expected method call.");
          break;
        }
        vmake_arguments args = arguments(gen);
        consume_expected(gen, TOKEN_RIGHT_PAREN, "Expected ')' after argument list");

        val = call_method(gen, *tmp, inst, &args);

        vmake_value_array_free(&args.args);
        vmake_table_free(&args.kwargs);
      } else {
        val = *tmp;
      }
    } else {
      break;
    }
  }

  return val;
}

vmake_value primary(vmake_gen *gen) {
  if (match(gen, TOKEN_FALSE)) {
    return vmake_value_bool(false);
  } else if (match(gen, TOKEN_TRUE)) {
    return vmake_value_bool(true);
  } else if (match(gen, TOKEN_NIL)) {
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
    return identifier_variable(gen);
  }

  error(gen, CTX_SYNTAX, "Expected expression.");
  return vmake_value_nil();
}

vmake_arguments arguments(vmake_gen *gen) {
  vmake_arguments arr;
  vmake_value_array_new(&arr.args);
  vmake_table_init(&arr.kwargs);
  bool read_args = false;
  do {
    if (check(gen, TOKEN_RIGHT_PAREN))
      break;

    if (match(gen, TOKEN_IDENTIFIER)) {
      vmake_token identifier_token = previous(gen);
      if (match(gen, TOKEN_EQUAL)) {
        vmake_value identifier = vmake_value_obj((vmake_obj *)vmake_obj_string_new(
            gen->state, (char *)identifier_token.name, identifier_token.name_length, true));
        vmake_value value = equality(gen);
        vmake_table_put_cpy(&arr.kwargs, identifier, value);
        read_args = true;
      } else {
        if (read_args) {
          error_at(gen, identifier_token, CTX_USER,
                   "Positional arguments must be placed before keyword arguments.");
        } else {
          vmake_value_array_push(&arr.args, *resolve_variable(gen, identifier_token));
        }
      }
    } else {
      vmake_value value = equality(gen);
      vmake_token value_token = previous(gen);
      if (read_args) {
        error_at(gen, value_token, CTX_USER,
                 "Positional arguments must be placed before keyword arguments.");
      } else {
        vmake_value_array_push(&arr.args, value);
      }
    }
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
  if (!check(gen, TOKEN_RIGHT_SQUARE_BRACKET)) {
    do {
      vmake_value_array_push(&arr, assignment(gen));
    } while (match(gen, TOKEN_COMMA));
  }
  consume_expected(gen, TOKEN_RIGHT_SQUARE_BRACKET, "Expected ']' after array.");
  vmake_obj *obj = (vmake_obj *)vmake_obj_array_new(gen->state, arr);
  push_obj(gen, obj);
  return vmake_value_obj(obj);
}

vmake_value number(vmake_gen *gen) { return vmake_value_number(atof(previous(gen).name)); }

vmake_value string(vmake_gen *gen) {
  vmake_token token = previous(gen);
  vmake_obj_string *obj =
      vmake_obj_string_new(gen->state, (char *)token.name, token.name_length, true);
  return vmake_value_obj((vmake_obj *)obj);
}

vmake_value identifier_variable(vmake_gen *gen) {
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

  // gen->stack[gen->stack_size++] = res;
  push(gen, res);
  return *res;
}

vmake_value identifier_string(vmake_gen *gen) { return string(gen); }

vmake_value assign_variable(vmake_gen *gen, vmake_value *lhs, vmake_value rhs) {
  *lhs = rhs;
  return *lhs;
}

vmake_value call_native(vmake_gen *gen, vmake_value val, vmake_arguments *args) {
  vmake_obj_native *native = (vmake_obj_native *)val.as.obj;
  if (native->arity != args->args.size) {
    char *str;
    char *native_name = vmake_obj_to_string((vmake_obj *)native);
    asprintf(&str, "Expected %i positional arguments for %s but found %i instead.", native->arity,
             native_name, args->args.size);
    free(native_name);
    error(gen, CTX_USER, str);
    free(str);
  }
  return native->function(gen, args);
}

vmake_value call_method(vmake_gen *gen, vmake_value val, vmake_obj_instance *caller,
                        vmake_arguments *args) {
  vmake_obj_method *method = (vmake_obj_method *)val.as.obj;
  if (method->arity != args->args.size) {
    char *str;
    char *method_name = vmake_obj_to_string((vmake_obj *)method);
    char *class_name = vmake_obj_to_string((vmake_obj *)caller->klass);
    asprintf(&str,
             "Expected %i positional arguments for method %s of class %s but found %i instead.",
             method->arity, method_name, class_name, args->args.size);
    free(class_name);
    free(method_name);
    error(gen, CTX_USER, str);
    free(str);
  }
  return method->method(caller, gen, args);
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
  return gen->stack_size == 0 ? (error(gen, CTX_INTERNAL, "Tried to pop from empty stack."), NULL)
                              : gen->stack[--gen->stack_size];
}

static void push(vmake_gen *gen, vmake_value *val) {
  if (gen->stack_size == 256) {
    error(gen, CTX_INTERNAL, "Stack overflow.");
  } else {
    gen->stack[gen->stack_size++] = val;
  }
}

static void push_obj(vmake_gen *gen, vmake_obj *obj) {
  vmake_value *val = malloc(sizeof(vmake_value));
  *val = vmake_value_obj(obj);
  push(gen, val);
}

static void modify_top(vmake_gen *gen, vmake_value *val) { gen->stack[gen->stack_size - 1] = val; }
