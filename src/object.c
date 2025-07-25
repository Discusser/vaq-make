#include "object.h"
#include "generator.h"
#include "table.h"
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

vmake_obj_string *vmake_obj_string_new(vmake_gen *gen, char *chars, int length, bool copy) {
  // Implementation of the FNV-1a algorithm. Constant values are taken from
  // http://www.isthe.com/chongo/tech/comp/fnv/#FNV-param
  uint32_t hash = 2166136261;
  for (int i = 0; i < length; i++) {
    hash = hash ^ chars[i];
    hash = hash * 16777619;
  }

  // If the string is interned, no point in allocating new memory.
  vmake_obj_string *interned = vmake_table_find_string(&gen->strings, chars, length, hash);
  if (interned != NULL) {
    // We only free the passed characters if the string is interned and we own
    // the characters, that is if we aren't copying the string, and the string
    // is not constant.
    if (!copy) {
      free(chars);
    }
    return interned;
  }

  vmake_obj_string *obj = OBJ_NEW(vmake_obj_string, OBJ_STRING);
  if (copy) {
    obj->chars = malloc(length + 1);
    memcpy(obj->chars, chars, length);
    obj->chars[length] = '\0';
  } else {
    obj->chars = chars;
  }
  obj->length = length;
  obj->hash = hash;

  // Intern the newly-created string
  vmake_table_put_ptr(&gen->strings, vmake_value_obj((vmake_obj *)obj), NULL);

  return obj;
}

void vmake_obj_string_free(vmake_obj_string *obj) {
  free(obj->chars);
  free(obj);
}

vmake_obj_native *vmake_obj_native_new(vmake_gen *gen, const char *name, int name_length,
                                       vmake_native_fn function, int arity) {
  vmake_obj_native *obj = OBJ_NEW(vmake_obj_native, OBJ_NATIVE);
  obj->arity = arity;
  obj->name = vmake_obj_string_new(gen, (char *)name, name_length, true);
  obj->function = function;
  return obj;
}

void vmake_obj_native_free(vmake_obj_native *obj) { free(obj); }
