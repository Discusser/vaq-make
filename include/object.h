#pragma once

#include "array.h"
#include "generator.h"
#include "table.h"
#include "value.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct vmake_gen vmake_gen;
typedef struct vmake_state vmake_state;
typedef struct vmake_obj_instance vmake_obj_instance;

typedef enum vmake_obj_type {
  OBJ_STRING,
  OBJ_NATIVE,
  OBJ_ARRAY,
  OBJ_CLASS,
  OBJ_INSTANCE,
  OBJ_METHOD,
} vmake_obj_type;

typedef struct vmake_obj {
  vmake_obj_type type;
  vmake_obj *next;
} vmake_obj;

typedef struct vmake_obj_string {
  vmake_obj obj;
  char *chars;
  int length;
  uint32_t hash;
} vmake_obj_string;

typedef struct vmake_arguments {
  vmake_value_array args;
  vmake_table kwargs;
} vmake_arguments;

typedef struct vmake_value (*vmake_native_function)(vmake_gen *gen, vmake_arguments *args);
typedef struct vmake_value (*vmake_native_method)(vmake_obj_instance *self, vmake_gen *gen,
                                                  vmake_arguments *args);

typedef struct vmake_obj_native {
  vmake_obj obj;
  vmake_obj_string *name;
  vmake_native_function function;
  int arity;
} vmake_obj_native;

typedef struct vmake_obj_array {
  vmake_obj obj;
  vmake_value_array *array;
} vmake_obj_array;

typedef struct vmake_obj_class {
  vmake_obj obj;
  vmake_obj_string *name;
  vmake_table methods;
} vmake_obj_class;

typedef struct vmake_obj_instance {
  vmake_obj obj;
  vmake_obj_class *klass;
  vmake_table fields;
} vmake_obj_instance;

typedef struct vmake_obj_method {
  vmake_obj obj;
  vmake_obj_string *name;
  vmake_native_method method;
  int arity;
} vmake_obj_method;

char *vmake_obj_type_to_string(vmake_obj_type type);

vmake_obj *vmake_obj_new(vmake_state *state, size_t size, vmake_obj_type type);
char *vmake_obj_to_string(vmake_obj *obj);
void vmake_obj_print(vmake_obj *obj);
void vmake_obj_free(vmake_obj *obj);

vmake_obj_string *vmake_obj_string_const(vmake_state *state, const char *str);
vmake_obj_string *vmake_obj_string_new(vmake_state *state, char *chars, int length, bool copy);
void vmake_obj_string_free(vmake_obj_string *obj);

vmake_obj_native *vmake_obj_native_new(vmake_state *state, const char *name,
                                       vmake_native_function function, int arity);
void vmake_obj_native_free(vmake_obj_native *obj);

vmake_obj_array *vmake_obj_array_new(vmake_state *state, vmake_value_array array);
void vmake_obj_array_free(vmake_obj_array *obj);

vmake_obj_class *vmake_obj_class_new(vmake_state *state, const char *name);
void vmake_obj_class_add_method(vmake_obj_class *obj, vmake_state *state, const char *name,
                                vmake_native_method method, int arity);
void vmake_obj_class_free(vmake_obj_class *obj);

vmake_obj_instance *vmake_obj_instance_new(vmake_state *state, vmake_obj_class *klass);
void vmake_obj_instance_add_field(vmake_obj_instance *obj, vmake_state *state, const char *name,
                                  vmake_value value);
void vmake_obj_instance_free(vmake_obj_instance *obj);

vmake_obj_method *vmake_obj_method_new(vmake_state *state, const char *name,
                                       vmake_native_method method, int arity);
void vmake_obj_method_free(vmake_obj_method *obj);
