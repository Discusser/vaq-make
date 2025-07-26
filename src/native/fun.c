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

static vmake_value expect_val(vmake_gen *gen, vmake_value val, vmake_value_type type) {
  if (val.type == type)
    return val;
  error(gen, "Expected %s but found %s instead.", vmake_value_type_to_string(type),
        vmake_value_type_to_string(val.type));
  return val;
}

static vmake_obj *expect_obj(vmake_gen *gen, vmake_value val, vmake_obj_type type) {
  expect_val(gen, val, VAL_OBJ);
  if (val.as.obj->type == type)
    return val.as.obj;
  error(gen, "Expected %s but found %s instead.", vmake_obj_type_to_string(type),
        vmake_obj_type_to_string(val.as.obj->type));
  return NULL;
}

void vmake_define_native_functions(vmake_state *state) {
  vmake_define_native_function(state, "executable", vmake_executable_native, 2);
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
