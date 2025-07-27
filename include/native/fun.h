#pragma once

#include "object.h"

void vmake_define_native_functions(vmake_state *state);
void vmake_define_native_function(vmake_state *state, const char *name, vmake_native_function fn,
                                  int argc);

// TODO: Start adding and implementing these functions. These are the core of the VMake language,
// because VMake in itself is not designed to be Turing complete.
vmake_value vmake_executable_native(vmake_gen *gen, vmake_arguments *args);
vmake_value vmake_get_properties_native(vmake_gen *gen, vmake_arguments *args);
