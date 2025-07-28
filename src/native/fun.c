#include "native/fun.h"
#include "common.h"
#include "file.h"
#include "object.h"
#include "table.h"
#include <stdlib.h>
#include <string.h>

static vmake_value expect_val(vmake_gen *gen, vmake_value val, vmake_value_type type);
static vmake_value expect_obj(vmake_gen *gen, const char *name, vmake_value val,
                              vmake_obj_type type);
static void make_paths_absolute(vmake_gen *gen, vmake_obj_array *paths);

#define error(gen, fmt, ...) vmake_error_exit(gen, CTX_NATIVE, NULL, fmt, __VA_ARGS__)
#define EXPECT_STR(name, val) ((vmake_obj_string *)expect_obj(gen, name, val, OBJ_STRING).as.obj)
#define EXPECT_ARR(name, val) ((vmake_obj_array *)expect_obj(gen, name, val, OBJ_ARRAY).as.obj)
#define EXPECT_ARR_OPT(name, val)                                                                  \
  (val.type == VAL_NIL ? vmake_value_nil() : expect_obj(gen, name, val, OBJ_ARRAY))
#define EXPECT_INST(name, val)                                                                     \
  ((vmake_obj_instance *)expect_obj(gen, name, val, OBJ_INSTANCE).as.obj)

static vmake_value expect_val(vmake_gen *gen, vmake_value val, vmake_value_type type) {
  if (val.type == type)
    return val;
  error(gen, "Expected %s but found %s instead.", vmake_value_type_to_string(type),
        vmake_value_type_to_string(val.type));
  return val;
}

static vmake_value expect_obj(vmake_gen *gen, const char *name, vmake_value val,
                              vmake_obj_type type) {
  if (val.type == VAL_OBJ && val.as.obj->type == type)
    return val;
  error(gen, "Expected %s for '%s' but found %s instead.", vmake_obj_type_to_string(type), name,
        val.type == VAL_OBJ ? vmake_obj_type_to_string(val.as.obj->type)
                            : vmake_value_type_to_string(val.type));
  return vmake_value_nil();
}

void vmake_define_native_functions(vmake_state *state) {
  vmake_define_native_function(state, "executable", vmake_executable_native, 1);
  vmake_define_native_function(state, "get_properties", vmake_get_properties_native, 1);
}

void vmake_define_native_function(vmake_state *state, const char *name, vmake_native_function fn,
                                  int argc) {
  vmake_obj_native *native = vmake_obj_native_new(state, name, fn, argc);
  vmake_table_put_cpy(&state->globals, vmake_value_obj((vmake_obj *)native->name),
                      vmake_value_obj((vmake_obj *)native));
}

vmake_value vmake_kwargs_get(vmake_gen *gen, vmake_table kwargs, const char *value) {
  vmake_value *val = NULL;
  vmake_table_get_or_nil(
      &kwargs, vmake_value_obj((vmake_obj *)vmake_obj_string_const(gen->state, value)), &val);
  return *val;
}

vmake_value vmake_executable_native(vmake_gen *gen, vmake_arguments *args) {
  vmake_obj_string *exe_name = EXPECT_STR("name", args->args.values[0]);
  vmake_obj_array *sources = EXPECT_ARR("sources", vmake_kwargs_get(gen, args->kwargs, "sources"));
  vmake_value include_directories = EXPECT_ARR_OPT(
      "include_directories", vmake_kwargs_get(gen, args->kwargs, "include_directories"));
  vmake_value link_libraries =
      EXPECT_ARR_OPT("link_libraries", vmake_kwargs_get(gen, args->kwargs, "link_libraries"));

  make_paths_absolute(gen, sources);
  if (include_directories.type != VAL_NIL) {
    make_paths_absolute(gen, (vmake_obj_array *)include_directories.as.obj);
  }

  vmake_obj_instance *inst =
      vmake_obj_instance_new(gen->state, gen->state->classes[CLASS_EXECUTABLE]);
  vmake_obj_instance_add_field(inst, gen->state, "name", vmake_value_obj((vmake_obj *)exe_name));
  vmake_obj_instance_add_field(inst, gen->state, "sources", vmake_value_obj((vmake_obj *)sources));
  vmake_obj_instance_add_field(inst, gen->state, "include_directories", include_directories);
  vmake_obj_instance_add_field(inst, gen->state, "link_libraries", link_libraries);

  vmake_value val = vmake_value_obj((vmake_obj *)inst);
  vmake_value_array_push(&gen->state->make.targets, val);
  return val;
}

vmake_value vmake_get_properties_native(vmake_gen *gen, vmake_arguments *args) {
  vmake_obj_instance *inst = EXPECT_INST("instance", args->args.values[0]);
  return vmake_value_obj((vmake_obj *)vmake_obj_table_new(gen->state, inst->fields));
}

static void make_paths_absolute(vmake_gen *gen, vmake_obj_array *paths) {
  for (int i = 0; i < paths->array->size; i++) {
    char *file_name = ((vmake_obj_string *)paths->array->values[i].as.obj)->chars;
    char *path_rel = vmake_path_rel(gen->file_path, file_name);
    char *path_abs = realpath(path_rel, NULL);
    if (!path_abs) {
      vmake_error_exit(gen, CTX_INTERNAL, NULL, "Could not find file at '%s'", file_name);
    }
    free(path_rel);
    paths->array->values[i].as.obj =
        (vmake_obj *)vmake_obj_string_new(gen->state, path_abs, strlen(path_abs), false);
  }
}
