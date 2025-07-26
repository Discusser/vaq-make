#pragma once

#include "array.h"
#include "native/class.h"
#include "object.h"
#include "scanner.h"
#include "value.h"
#include <stdbool.h>

typedef struct vmake_obj_class vmake_obj_class;

typedef struct vmake_variable {
  vmake_value value;
  vmake_token name;
  int depth;
} vmake_variable;

bool vmake_generate_build(vmake_scanner *scanner, vmake_state *state, const char *file_path);
