#pragma once

#include "array.h"
#include "scanner.h"
#include "table.h"
#include "value.h"
#include <stdbool.h>

typedef struct vmake_variable {
  vmake_value value;
  vmake_token name;
  int depth;
} vmake_variable;

typedef struct vmake_state {
  vmake_obj *objects;
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

bool vmake_generate_build(vmake_scanner *scanner, vmake_state *state, const char *file_path);
