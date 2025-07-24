#pragma once

#include "value.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum vaq_make_obj_type {
  OBJ_STRING,
  OBJ_NATIVE,
} vaq_make_obj_type;

typedef struct vaq_make_obj {
  vaq_make_obj_type type;
} vaq_make_obj;

typedef struct vaq_make_obj_string {
  vaq_make_obj obj;
  int length;
  char *chars;
} vaq_make_obj_string;

typedef struct vaq_make_value vaq_make_value;

typedef vaq_make_value (*vaq_make_native_fn)(int argc, vaq_make_value *argv);

typedef struct vaq_make_obj_native {
  vaq_make_obj obj;
  vaq_make_native_fn function;
} vaq_make_obj_native;

vaq_make_obj *vaq_make_obj_new(size_t size, vaq_make_obj_type type);
char *vaq_make_obj_to_string(vaq_make_obj *obj);
void vaq_make_obj_print(vaq_make_obj *obj);
void vaq_make_obj_free(vaq_make_obj *obj);

vaq_make_obj_string *vaq_make_obj_string_new(char *chars, int length, bool copy);
void vaq_make_obj_string_free(vaq_make_obj_string *obj);

vaq_make_obj_native *vaq_make_obj_native_new(vaq_make_native_fn function);
void vaq_make_obj_native_free(vaq_make_obj_native *obj);
