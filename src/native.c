#include "native.h"
#include "generator.h"
#include "object.h"
#include "table.h"
#include <stdio.h>
#include <string.h>

void vmake_define_natives(vmake_gen *gen) {
  vmake_define_native(gen, "executable", vmake_executable_native, 1);
}

void vmake_define_native(vmake_gen *gen, const char *name, vmake_native_fn fn, int argc) {
  int name_length = strlen(name);
  vmake_obj_string *key = vmake_obj_string_new(gen, (char *)name, name_length, true);
  vmake_obj_native *native = vmake_obj_native_new(gen, name, name_length, fn, argc);
  vmake_table_put_cpy(&gen->globals, vmake_value_obj((vmake_obj *)key),
                      vmake_value_obj((vmake_obj *)native));
}

vmake_value vmake_executable_native(int argc, vmake_value *argv) {
  printf("Hello world! argc = %i\n", argc);
  return vmake_value_nil();
}
