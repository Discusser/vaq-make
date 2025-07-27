#include "native/class.h"
#include "common.h"
#include <stdio.h>

static vmake_value *get_field_or_nil(vmake_gen *gen, vmake_obj_instance *inst, const char *name);

static void define_Executable_native(vmake_state *state);
static vmake_value Executable_get_sources(vmake_obj_instance *self, vmake_gen *gen,
                                          vmake_arguments *args);

void vmake_define_native_classes(vmake_state *state) { define_Executable_native(state); }

void vmake_define_native_class(vmake_state *state, vmake_obj_class *klass) {
  vmake_table_put_cpy(&state->globals, vmake_value_obj((vmake_obj *)klass->name),
                      vmake_value_obj((vmake_obj *)klass));
}

static vmake_value *get_field_or_nil(vmake_gen *gen, vmake_obj_instance *inst, const char *name) {
  vmake_value key = vmake_value_obj((vmake_obj *)vmake_obj_string_const(gen->state, "sources"));
  vmake_value *value;
  vmake_table_get_or_nil(&inst->fields, key, &value);
  return value;
}

static void define_Executable_native(vmake_state *state) {
  vmake_obj_class *klass = vmake_obj_class_new(state, "Executable");

  vmake_obj_class_add_method(klass, state, "my_method", Executable_get_sources, 0);

  vmake_define_native_class(state, klass);
  state->classes[CLASS_EXECUTABLE] = klass;
}

static vmake_value Executable_get_sources(vmake_obj_instance *self, vmake_gen *gen,
                                          vmake_arguments *args) {
  return *get_field_or_nil(gen, self, "sources");
}
