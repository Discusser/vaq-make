#pragma once

#include "object.h"

void vmake_define_natives(vmake_gen *gen);
void vmake_define_native(vmake_gen *gen, const char *name, vmake_native_fn fn, int argc);

// TODO: Start adding and implementing these functions. These are the core of the VMake language,
// because VMake in itself is not supposed to be Turing complete.
vmake_value vmake_executable_native(int argc, vmake_value *argv);
