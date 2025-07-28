#pragma once

#include "generator.h"
#include "value.h"
#include <stdio.h>

#define VMAKE_STRING_BUF_INITIAL_SIZE 8
#define VMAKE_STRING_BUF_GROW_FACTOR 2

typedef enum vmake_class_type { CLASS_EXECUTABLE, CLASS_T_MAX } vmake_class_type;

typedef struct vmake_makefile {
  FILE *fp;
  char *name;
  char *dir_path;
} vmake_makefile;

typedef struct vmake_make_contents {
  char *build_directory;
  char *source_directory;
  vmake_value_array targets;
} vmake_make_contents;

typedef struct vmake_state {
  vmake_obj *objects;
  vmake_obj_class *classes[CLASS_T_MAX];
  vmake_table globals;
  vmake_table strings;
  vmake_value_array include_stack;
  vmake_make_contents make;
  char **argv;
  char *root_file;
  int argc;
  bool had_error;
  bool panic_mode;
} vmake_state;

typedef struct vmake_gen {
  const char *file_path;
  vmake_state *state;
  vmake_scanner *scanner;
  vmake_variable_array locals;
  vmake_value *stack[256];
  vmake_token previous;
  vmake_token current;
  int stack_size;
  int scope_depth;
} vmake_gen;

typedef struct vmake_string_buf {
  char *string;
  int size;
  int capacity;
} vmake_string_buf;

typedef enum vmake_error_context {
  // For errors that come from invalid syntax written by users
  CTX_SYNTAX,
  // For errors that come from invalid code written by users, such as invalid use of types
  CTX_USER,
  // For errors that come from native functions
  CTX_NATIVE,
  // For errors that come from the vaq-make itself. You better hope there are no errors of this
  // type.
  CTX_INTERNAL,
} vmake_error_context;

void vmake_process_path(vmake_state *state, char *path);

void vmake_verror(vmake_gen *gen, vmake_error_context context, vmake_token *token, const char *fmt,
                  va_list ap);
// General function for printing an error. If the error is not part of some VMake code, token can be
// passed as NULL. gen can also be NULL, usually when context is CTX_NATIVE or CTX_INTERNAL and the
// function doesn't have access to a gen variable.
void vmake_error(vmake_gen *gen, vmake_error_context context, vmake_token *token, const char *fmt,
                 ...);
// Prints an error and then exits. This should only be called if the error will place the program in
// a broken state, and cause a crash.
void vmake_error_exit(vmake_gen *gen, vmake_error_context context, vmake_token *token,
                      const char *fmt, ...);

void vmake_string_buf_new(vmake_string_buf *buf);
void vmake_string_buf_append(vmake_string_buf *buf, const char *fmt, ...);
void vmake_string_buf_free(vmake_string_buf *buf);
