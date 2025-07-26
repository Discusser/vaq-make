#pragma once

#include "generator.h"
#include "value.h"

typedef enum vmake_class_type { CLASS_EXECUTABLE, CLASS_T_MAX } vmake_class_type;

typedef struct vmake_state {
  vmake_obj *objects;
  vmake_obj_class *classes[CLASS_T_MAX];
  vmake_table globals;
  vmake_table strings;
  vmake_value_array include_stack;
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

// General function for printing an error. If the error is not part of some VMake code, token can be
// passed as NULL
void vmake_error(vmake_gen *gen, vmake_error_context context, vmake_token *token, const char *fmt,
                 ...);
// Prints an error and then exits. This should only be called if the error will place the program in
// a broken state, and cause a crash.
void vmake_error_exit(vmake_gen *gen, vmake_error_context context, vmake_token *token,
                      const char *fmt, ...);
