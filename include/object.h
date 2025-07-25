#pragma once

#include "value.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct vmake_gen vmake_gen;

typedef enum vmake_obj_type {
  OBJ_STRING,
  OBJ_NATIVE,
} vmake_obj_type;

typedef struct vmake_obj {
  vmake_obj_type type;
} vmake_obj;

typedef struct vmake_obj_string {
  vmake_obj obj;
  int length;
  char *chars;
  uint32_t hash;
} vmake_obj_string;

typedef struct vmake_value vmake_value;

typedef vmake_value (*vmake_native_fn)(int argc, vmake_value *argv);

typedef struct vmake_obj_native {
  vmake_obj obj;
  int arity;
  vmake_obj_string *name;
  vmake_native_fn function;
} vmake_obj_native;

vmake_obj *vmake_obj_new(size_t size, vmake_obj_type type);
char *vmake_obj_to_string(vmake_obj *obj);
void vmake_obj_print(vmake_obj *obj);
void vmake_obj_free(vmake_obj *obj);

vmake_obj_string *vmake_obj_string_new(vmake_gen *gen, char *chars, int length, bool copy);
void vmake_obj_string_free(vmake_obj_string *obj);

vmake_obj_native *vmake_obj_native_new(vmake_gen *gen, const char *name, int name_length,
                                       vmake_native_fn function, int arity);
void vmake_obj_native_free(vmake_obj_native *obj);
