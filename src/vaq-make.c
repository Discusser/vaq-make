#include "common.h"
#include "generator.h"
#include "object.h"
#include "scanner-priv.h"
#include "scanner.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s [vmake_file]\n", argv[0]);
    exit(1);
  }

  // TODO: An issue arises because of the fact that the compiler and interpreter are not separated.
  // At the moment, when vmake_error is called, the actual VMake program might be left in an invalid
  // state, yet it will continue running. For example, I can run the following:
  //
  // some_variable = "test";
  // print(some_variable());
  //
  // and this will print an error saying that the object is not callable, followed by "test". The
  // reason being that the program doesn't stop executing. The sanest option is most likely to
  // immediately exit after any sort of error, whether it be a syntax error or a code error, which
  // is similar to what python does.
  //
  // TODO: Add tests

  vmake_state state;
  vmake_table_init(&state.globals);
  vmake_table_init(&state.strings);
  vmake_value_array_new(&state.include_stack);
  state.had_error = false;
  state.panic_mode = false;
  state.objects = NULL;

  char *path = realpath(argv[1], NULL);
  vmake_process_path(&state, path);
  free(path);

  vmake_value_array_free(&state.include_stack);
  vmake_table_free(&state.strings);
  vmake_table_free(&state.globals);

  return 0;
}

void vmake_process_path(vmake_state *state, char *path) {
  char *source = read_file(path, NULL);
  vmake_scanner scanner = vmake_init_scanner(source);
  vmake_generate_build(&scanner, state, path);
  free(source);
}

void vmake_error(vmake_gen *gen, vmake_error_context context, vmake_token *token, const char *fmt,
                 ...) {
  if (gen->state->panic_mode)
    return;
  gen->state->panic_mode = true;

  const char *filename;
  switch (context) {
  case CTX_SYNTAX:
  case CTX_USER:
    filename = gen->file_path;
    break;
  case CTX_NATIVE:
    filename = "native";
    break;
  case CTX_INTERNAL:
    filename = "internal";
    break;
  }
  fprintf(stderr, "[%s", filename);
  if (context == CTX_SYNTAX || context == CTX_USER) {
    // Technically token should only be NULL if the context is different from CTX_SYNTAX or CTX_USER
    // but this simplifies the code for us, since we don't really want to throw an error from the
    // error function itself.
    if (token == NULL || token->type == TOKEN_NONE) {
      fprintf(stderr, "] ERROR: ");
    } else {
      fprintf(stderr, ":%d] ERROR ", token->line + 1);
      if (token->type == TOKEN_EOF) {
        fprintf(stderr, "at end");
      } else if (token->type != TOKEN_ERROR) {
        fprintf(stderr, "at '%.*s'", token->name_length, token->name);
      }
      fprintf(stderr, ": ");
    }
  } else {
    fprintf(stderr, "] ERROR: ");
  }
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
  fprintf(stderr, "\n");

  for (int i = gen->state->include_stack.size - 2; i >= 0; i--) {
    char *str = vmake_value_to_string(gen->state->include_stack.values[i]);
    printf("  included from '%s'\n", str);
    free(str);
  }

  gen->state->had_error = true;
}

void vmake_error_exit(vmake_gen *gen, vmake_error_context context, vmake_token *token,
                      const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  vmake_error(gen, context, token, fmt, va);
  va_end(va);

  exit(1);
}
