#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OBJ_NEW(struct_t, type) (struct_t *)vmake_obj_new(sizeof(struct_t), type)

vmake_obj *vmake_obj_new(size_t size, vmake_obj_type type) {
  vmake_obj *obj = malloc(size);
  obj->type = type;
  return obj;
}

char *vmake_obj_to_string(vmake_obj *obj) {
  switch (obj->type) {
  case OBJ_STRING: {
    vmake_obj_string *str = (vmake_obj_string *)obj;
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

void vmake_obj_print(vmake_obj *obj) {
  char *buf = vmake_obj_to_string(obj);
  printf("%s", buf);
  free(buf);
}

void vmake_obj_free(vmake_obj *obj) { free(obj); }

vmake_obj_string *vmake_obj_string_new(char *chars, int length, bool copy) {
  vmake_obj_string *obj = OBJ_NEW(vmake_obj_string, OBJ_STRING);

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

void vmake_obj_string_free(vmake_obj_string *obj) {
  free(obj->chars);
  free(obj);
}

vmake_obj_native *vmake_obj_native_new(vmake_native_fn function) {
  vmake_obj_native *obj = OBJ_NEW(vmake_obj_native, OBJ_NATIVE);
  obj->function = function;
  return obj;
}

void vmake_obj_native_free(vmake_obj_native *obj) { free(obj); }
