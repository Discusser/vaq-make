#pragma once

#include "array.h"
#include "scanner.h"
#include "value.h"
#include <stdbool.h>

typedef struct vaq_make_variable {
  vaq_make_value value;
  vaq_make_token name;
} vaq_make_variable;

typedef struct vaq_make_gen {
  vaq_make_scanner *scanner;
  vaq_make_variable_array variables;
  vaq_make_value *stack[256];
  vaq_make_token previous;
  vaq_make_token current;
  int stack_size;
  bool had_error;
  bool panic_mode;
} vaq_make_gen;

bool vaq_make_generate_build(vaq_make_scanner *scanner);
