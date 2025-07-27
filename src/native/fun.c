#include "native/fun.h"
#include "common.h"
#include "object.h"
#include "table.h"
#include <string.h>

static vmake_value expect_val(vmake_gen *gen, vmake_value val, vmake_value_type type);
static vmake_obj *expect_obj(vmake_gen *gen, vmake_value val, vmake_obj_type type);

#define error(gen, fmt, ...) vmake_error_exit(gen, CTX_NATIVE, NULL, fmt, __VA_ARGS__)
#define EXPECT_STR(val) (vmake_obj_string *)expect_obj(gen, val, OBJ_STRING)
#define EXPECT_ARR(val) (vmake_obj_array *)expect_obj(gen, val, OBJ_ARRAY)
#define EXPECT_INST(val) (vmake_obj_instance *)expect_obj(gen, val, OBJ_INSTANCE)

static vmake_value expect_val(vmake_gen *gen, vmake_value val, vmake_value_type type) {
  if (val.type == type)
    return val;
  error(gen, "Expected %s but found %s instead.", vmake_value_type_to_string(type),
        vmake_value_type_to_string(val.type));
  return val;
}

static vmake_obj *expect_obj(vmake_gen *gen, vmake_value val, vmake_obj_type type) {
  if (val.type == VAL_OBJ && val.as.obj->type == type)
    return val.as.obj;
  error(gen, "Expected %s but found %s instead.", vmake_obj_type_to_string(type),
        val.type == VAL_OBJ ? vmake_obj_type_to_string(val.as.obj->type)
                            : vmake_value_type_to_string(val.type));
  return NULL;
}

void vmake_define_native_functions(vmake_state *state) {
  vmake_define_native_function(state, "executable", vmake_executable_native, 2);
  vmake_define_native_function(state, "get_properties", vmake_get_properties_native, 1);
}

void vmake_define_native_function(vmake_state *state, const char *name, vmake_native_function fn,
                                  int argc) {
  vmake_obj_native *native = vmake_obj_native_new(state, name, fn, argc);
  vmake_table_put_cpy(&state->globals, vmake_value_obj((vmake_obj *)native->name),
                      vmake_value_obj((vmake_obj *)native));
}

vmake_value vmake_executable_native(vmake_gen *gen, vmake_arguments *args) {
  vmake_obj_string *exe_name = EXPECT_STR(args->args.values[0]);
  vmake_obj_array *sources = EXPECT_ARR(args->args.values[1]);

  vmake_obj_instance *inst =
      vmake_obj_instance_new(gen->state, gen->state->classes[CLASS_EXECUTABLE]);
  vmake_obj_instance_add_field(inst, gen->state, "sources", vmake_value_obj((vmake_obj *)sources));
  return vmake_value_obj((vmake_obj *)inst);
}

vmake_value vmake_get_properties_native(vmake_gen *gen, vmake_arguments *args) {
  vmake_obj_instance *inst = EXPECT_INST(args->args.values[0]);
  return vmake_value_obj((vmake_obj *)vmake_obj_table_new(gen->state, inst->fields));
}
