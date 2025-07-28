#include "common.h"
#include "config.h"
#include "file.h"
#include "generator.h"
#include "object.h"
#include "scanner-priv.h"
#include "scanner.h"
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 2 && argc != 4) {
    printf("Usage: %s [vmake_file] [source_directory] [build_directory]\n", argv[0]);
    exit(1);
  }

  vmake_state state;
  vmake_table_init(&state.globals);
  vmake_table_init(&state.strings);
  vmake_value_array_new(&state.include_stack);
  vmake_value_array_new(&state.make.targets);
  state.had_error = false;
  state.panic_mode = false;
  state.objects = NULL;
  state.argc = argc;
  state.argv = argv;

  argv[1] = realpath(argv[1], NULL);
  state.root_file = argv[1];
  vmake_process_path(&state, argv[1]);
  char *path_copy = strdup(argv[1]);
  if (argc == 4) {
    char *src = realpath(argv[2], NULL);
    if (src == NULL)
      vmake_error_exit(NULL, CTX_USER, NULL, "Directory at '%s' doesn't exist", argv[2]);
    argv[2] = src;
    char *build = realpath(argv[3], NULL);
    if (build == NULL)
      vmake_error_exit(NULL, CTX_USER, NULL, "Directory at '%s' doesn't exist", argv[3]);
    argv[3] = build;
    vmake_build_makefiles(&state, argv[3], argv[2]);
    free(argv[2]);
    free(argv[3]);
  } else {
    vmake_build_makefiles(&state, dirname(path_copy), ".");
  }
  free(path_copy);
  free(argv[1]);

  vmake_value_array_free(&state.make.targets);
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

void vmake_verror(vmake_gen *gen, vmake_error_context context, vmake_token *token, const char *fmt,
                  va_list ap) {
  if (gen) {
    if (gen->state->panic_mode)
      return;
    gen->state->panic_mode = true;
  }

  char *filename;
  bool filename_is_path = false;
  switch (context) {
  case CTX_SYNTAX:
  case CTX_USER:
    filename = gen ? (filename_is_path = true, vmake_path_abs_to_rel(gen->file_path)) : "unknown";
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
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  if (filename_is_path)
    free(filename);
  if (gen) {
    for (int i = gen->state->include_stack.size - 2; i >= 0; i--) {
      char *str = ((vmake_obj_string *)gen->state->include_stack.values[i].as.obj)->chars;
      char *rel_path = vmake_path_abs_to_rel(str);
      printf("  included from %s\n", rel_path);
      free(rel_path);
      free(str);
    }

    gen->state->had_error = true;
  }
}

void vmake_error(vmake_gen *gen, vmake_error_context context, vmake_token *token, const char *fmt,
                 ...) {
  va_list va;
  va_start(va, fmt);
  vmake_verror(gen, context, token, fmt, va);
  va_end(va);
}

void vmake_error_exit(vmake_gen *gen, vmake_error_context context, vmake_token *token,
                      const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  vmake_verror(gen, context, token, fmt, va);
  va_end(va);

  exit(1);
}

void vmake_string_buf_new(vmake_string_buf *buf) {
  buf->string = malloc(sizeof(char) * VMAKE_STRING_BUF_INITIAL_SIZE);
  buf->string[0] = '\0';
  buf->size = 0;
  buf->capacity = VMAKE_STRING_BUF_INITIAL_SIZE;
}

void vmake_string_buf_append(vmake_string_buf *buf, const char *fmt, ...) {
  va_list ap;

  // Get the required size to print the text
  va_start(ap, fmt);
  int n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);

  if (n < 0)
    fprintf(stderr, "An error occurred while trying to append to a string buffer.");

  if (n >= buf->capacity) {
    buf->capacity *= 2;
    if (n >= buf->capacity) {
      buf->capacity = n + 1; // + 1 for null terminating byte
    }

    buf->string = realloc(buf->string, buf->capacity);
  }

  // Now that we've potentially grown our buffer, we can safely print to the buffer.
  va_start(ap, fmt);
  vsnprintf(buf->string + buf->size, n + 1, fmt, ap);
  buf->size += n;
  va_end(ap);
}

void vmake_string_buf_free(vmake_string_buf *buf) { free(buf->string); }
