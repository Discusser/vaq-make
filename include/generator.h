#pragma once

#include "array.h"
#include "scanner.h"
#include "value.h"
#include <stdbool.h>

typedef struct vmake_variable {
  vmake_value value;
  vmake_token name;
} vmake_variable;

typedef struct vmake_gen {
  const char *file_name;
  vmake_scanner *scanner;
  vmake_variable_array variables;
  vmake_value *stack[256];
  vmake_token previous;
  vmake_token current;
  int stack_size;
  bool had_error;
  bool panic_mode;
} vmake_gen;

bool vmake_generate_build(vmake_scanner *scanner, const char *file_path);
