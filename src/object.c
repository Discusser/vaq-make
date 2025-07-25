#include "object.h"
#include "generator.h"
#include "table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OBJ_NEW(struct_t, type) (struct_t *)vmake_obj_new(state, sizeof(struct_t), type)

vmake_obj *vmake_obj_new(vmake_state *state, size_t size, vmake_obj_type type) {
  vmake_obj *obj = malloc(size);
  obj->type = type;

  if (state->objects == NULL)
    state->objects = obj;
  else {
    obj->next = state->objects;
    state->objects = obj;
  }

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
    asprintf(&buf, "<native '%s'>", ((vmake_obj_native *)obj)->name->chars);
    return buf;
  }
  case OBJ_ARRAY: {
    vmake_value_array *arr = ((vmake_obj_array *)obj)->array;
    char *buf = malloc(sizeof(char) * 2);
    buf[0] = '[';
    int len = 1;
    for (int i = 0; i < arr->size; i++) {
      char *tmp = vmake_value_to_string(arr->values[i]);
      int tmp_len = strlen(tmp);
      buf = realloc(buf, len + tmp_len + 4);
      memcpy(buf + len, tmp, tmp_len);
      len += tmp_len;
      if (i != arr->size - 1) {
        memcpy(buf + len, ", ", 2);
        len += 2;
      }
    }
    buf[len] = ']';
    buf[len + 1] = '\0';
    return buf;
  }
  default: {
    char *buf;
    asprintf(&buf, "<obj %p>", obj);
    return buf;
  }
  }
}

void vmake_obj_print(vmake_obj *obj) {
  char *buf = vmake_obj_to_string(obj);
  printf("%s", buf);
  free(buf);
}

void vmake_obj_free(vmake_obj *obj) { free(obj); }

vmake_obj_string *vmake_obj_string_new(vmake_state *state, char *chars, int length, bool copy) {
  // Implementation of the FNV-1a algorithm. Constant values are taken from
  // http://www.isthe.com/chongo/tech/comp/fnv/#FNV-param
  uint32_t hash = 2166136261;
  for (int i = 0; i < length; i++) {
    hash = hash ^ chars[i];
    hash = hash * 16777619;
  }

  // If the string is interned, no point in allocating new memory.
  vmake_obj_string *interned = vmake_table_find_string(&state->strings, chars, length, hash);
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
  vmake_table_put_ptr(&state->strings, vmake_value_obj((vmake_obj *)obj), NULL);

  return obj;
}

void vmake_obj_string_free(vmake_obj_string *obj) {
  free(obj->chars);
  free(obj);
}

vmake_obj_native *vmake_obj_native_new(vmake_state *state, const char *name, int name_length,
                                       vmake_native_fn function, int arity) {
  vmake_obj_native *obj = OBJ_NEW(vmake_obj_native, OBJ_NATIVE);
  obj->arity = arity;
  obj->name = vmake_obj_string_new(state, (char *)name, name_length, true);
  obj->function = function;
  return obj;
}

void vmake_obj_native_free(vmake_obj_native *obj) { free(obj); }

vmake_obj_array *vmake_obj_array_new(vmake_state *state, vmake_value_array array) {
  vmake_obj_array *obj = OBJ_NEW(vmake_obj_array, OBJ_ARRAY);
  obj->array = malloc(sizeof(vmake_value_array));
  *obj->array = array;
  return obj;
}

void vmake_obj_array_free(vmake_obj_array *obj) {
  free(obj->array);
  free(obj);
}
