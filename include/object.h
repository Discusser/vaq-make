#pragma once

#include "array.h"
#include "generator.h"
#include "value.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct vmake_gen vmake_gen;
typedef struct vmake_value vmake_value;
typedef struct vmake_value_array vmake_value_array;

typedef enum vmake_obj_type {
  OBJ_STRING,
  OBJ_NATIVE,
  OBJ_ARRAY,
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

typedef struct vmake_value (*vmake_native_fn)(int argc, vmake_value *argv);

typedef struct vmake_obj_native {
  vmake_obj obj;
  vmake_obj_string *name;
  vmake_native_fn function;
  int arity;
} vmake_obj_native;

typedef struct vmake_obj_array {
  vmake_obj obj;
  vmake_value_array *array;
} vmake_obj_array;

vmake_obj *vmake_obj_new(vmake_state *state, size_t size, vmake_obj_type type);
char *vmake_obj_to_string(vmake_obj *obj);
void vmake_obj_print(vmake_obj *obj);
void vmake_obj_free(vmake_obj *obj);

vmake_obj_string *vmake_obj_string_new(vmake_state *state, char *chars, int length, bool copy);
void vmake_obj_string_free(vmake_obj_string *obj);

vmake_obj_native *vmake_obj_native_new(vmake_state *state, const char *name, int name_length,
                                       vmake_native_fn function, int arity);
void vmake_obj_native_free(vmake_obj_native *obj);

vmake_obj_array *vmake_obj_array_new(vmake_state *state, vmake_value_array array);
void vmake_obj_array_free(vmake_obj_array *obj);
