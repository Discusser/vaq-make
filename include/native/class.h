#pragma once

#include "generator.h"
#include "object.h"

typedef struct vmake_state vmake_state;
typedef struct vmake_obj_class vmake_obj_class;

void vmake_define_native_classes(vmake_state *state);
void vmake_define_native_class(vmake_state *state, vmake_obj_class *klass);
