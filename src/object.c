#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OBJ_NEW(struct_t, type) (struct_t *)vaq_make_obj_new(sizeof(struct_t), type)

vaq_make_obj *vaq_make_obj_new(size_t size, vaq_make_obj_type type) {
  vaq_make_obj *obj = malloc(size);
  obj->type = type;
  return obj;
}

char *vaq_make_obj_to_string(vaq_make_obj *obj) {
  switch (obj->type) {
  case OBJ_STRING: {
    vaq_make_obj_string *str = (vaq_make_obj_string *)obj;
    char *buf = malloc(str->length + 1);
    memcpy(buf, str->chars, str->length);
    buf[str->length] = '\0';
    return buf;
  }
  case OBJ_NATIVE: {
    char *buf;
    asprintf(&buf, "%s", "<native>");
    return buf;
  }
  }

  return NULL;
}

void vaq_make_obj_print(vaq_make_obj *obj) {
  char *buf = vaq_make_obj_to_string(obj);
  printf("%s", buf);
  free(buf);
}

void vaq_make_obj_free(vaq_make_obj *obj) { free(obj); }

vaq_make_obj_string *vaq_make_obj_string_new(char *chars, int length, bool copy) {
  vaq_make_obj_string *obj = OBJ_NEW(vaq_make_obj_string, OBJ_STRING);

  if (copy) {
    obj->chars = malloc(length + 1);
    memcpy(obj->chars, chars, length);
    obj->chars[length] = '\0';
  } else {
    obj->chars = chars;
  }
  obj->length = length;

  return obj;
}

void vaq_make_obj_string_free(vaq_make_obj_string *obj) {
  free(obj->chars);
  free(obj);
}

vaq_make_obj_native *vaq_make_obj_native_new(vaq_make_native_fn function) {
  vaq_make_obj_native *obj = OBJ_NEW(vaq_make_obj_native, OBJ_NATIVE);
  obj->function = function;
  return obj;
}

void vaq_make_obj_native_free(vaq_make_obj_native *obj) { free(obj); }
