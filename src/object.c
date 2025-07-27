#include "object.h"
#include "common.h"
#include "generator.h"
#include "table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OBJ_NEW(struct_t, type) (struct_t *)vmake_obj_new(state, sizeof(struct_t), type)

char *vmake_obj_type_to_string(vmake_obj_type type) {
  switch (type) {
  case OBJ_STRING:
    return "string";
  case OBJ_NATIVE:
    return "native";
  case OBJ_ARRAY:
    return "array";
  case OBJ_CLASS:
    return "class";
  case OBJ_INSTANCE:
    return "instance";
  case OBJ_METHOD:
    return "method";
  case OBJ_TABLE:
    return "table";
  }

  return "obj unknown";
}

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
    vmake_string_buf buf;
    vmake_string_buf_new(&buf);
    vmake_string_buf_append(&buf, "\"%.*s\"", str->length, str->chars);
    return buf.string;
  }
  case OBJ_NATIVE: {
    char *buf;
    asprintf(&buf, "<native %s>",
             vmake_obj_to_string((vmake_obj *)((vmake_obj_native *)obj)->name));
    return buf;
  }
  case OBJ_ARRAY: {
    vmake_value_array *arr = ((vmake_obj_array *)obj)->array;
    vmake_string_buf buf;
    vmake_string_buf_new(&buf);
    vmake_string_buf_append(&buf, "[");
    int len = 1;
    for (int i = 0; i < arr->size; i++) {
      char *element = vmake_value_to_string(arr->values[i]);
      vmake_string_buf_append(&buf, "%s", element);
      free(element);
      if (i != arr->size - 1) {
        vmake_string_buf_append(&buf, ", ");
      }
    }
    vmake_string_buf_append(&buf, "]");
    return buf.string;
  }
  case OBJ_CLASS: {
    char *class_name = vmake_obj_to_string((vmake_obj *)((vmake_obj_class *)obj)->name);
    char *buf;
    asprintf(&buf, "<class %s>", class_name);
    free(class_name);
    return buf;
  }
  case OBJ_INSTANCE: {
    char *class_name = vmake_obj_to_string((vmake_obj *)((vmake_obj_instance *)obj)->klass->name);
    char *buf;
    asprintf(&buf, "<instance %s>", class_name);
    free(class_name);
    return buf;
  }
  case OBJ_METHOD: {
    char *method_name = vmake_obj_to_string((vmake_obj *)((vmake_obj_method *)obj)->name);
    char *buf;
    asprintf(&buf, "<method %s>", method_name);
    free(method_name);
    return buf;
  }
  case OBJ_TABLE: {
    vmake_table *table = ((vmake_obj_table *)obj)->table;
    vmake_string_buf buf;
    vmake_string_buf_new(&buf);
    vmake_string_buf_append(&buf, "{");
    bool first = true;
    for (int i = 0; i < table->capacity; i++) {
      vmake_table_entry entry = table->entries[i];
      if (entry.key.type != VAL_EMPTY) {
        char *key = vmake_value_to_string(entry.key);
        char *value = vmake_value_to_string(*entry.value);
        if (!first)
          vmake_string_buf_append(&buf, ", ");
        vmake_string_buf_append(&buf, "%s=%s", key, value);
        free(value);
        free(key);
        first = false;
      }
    }
    vmake_string_buf_append(&buf, "}");
    return buf.string;
    break;
  }
  default: {
    char *buf;
    asprintf(&buf, "<obj %s>", vmake_obj_type_to_string(obj->type));
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

vmake_obj_string *vmake_obj_string_const(vmake_state *state, const char *str) {
  return vmake_obj_string_new(state, (char *)str, strlen(str), true);
}

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

vmake_obj_native *vmake_obj_native_new(vmake_state *state, const char *name,
                                       vmake_native_function function, int arity) {
  vmake_obj_native *obj = OBJ_NEW(vmake_obj_native, OBJ_NATIVE);
  obj->arity = arity;
  obj->name = vmake_obj_string_new(state, (char *)name, strlen(name), true);
  obj->function = function;
  return obj;
}

void vmake_obj_native_free(vmake_obj_native *obj) {
  vmake_obj_string_free(obj->name);
  free(obj);
}

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

vmake_obj_class *vmake_obj_class_new(vmake_state *state, const char *name) {
  vmake_obj_class *obj = OBJ_NEW(vmake_obj_class, OBJ_CLASS);
  obj->name = vmake_obj_string_new(state, (char *)name, strlen(name), true);
  vmake_table_init(&obj->methods);
  return obj;
}

void vmake_obj_class_add_method(vmake_obj_class *obj, vmake_state *state, const char *name,
                                vmake_native_method method, int arity) {
  vmake_value key = vmake_value_obj((vmake_obj *)vmake_obj_string_const(state, name));
  vmake_value value =
      vmake_value_obj((vmake_obj *)vmake_obj_method_new(state, name, method, arity));
  vmake_table_put_cpy(&obj->methods, key, value);
}

void vmake_obj_class_free(vmake_obj_class *obj) {
  vmake_obj_string_free(obj->name);
  free(obj);
}

vmake_obj_instance *vmake_obj_instance_new(vmake_state *state, vmake_obj_class *klass) {
  vmake_obj_instance *obj = OBJ_NEW(vmake_obj_instance, OBJ_INSTANCE);
  obj->klass = klass;
  vmake_table_init(&obj->fields);
  return obj;
}

void vmake_obj_instance_add_field(vmake_obj_instance *obj, vmake_state *state, const char *name,
                                  vmake_value value) {
  vmake_value key = vmake_value_obj((vmake_obj *)vmake_obj_string_const(state, name));
  vmake_table_put_cpy(&obj->fields, key, value);
}

void vmake_obj_instance_free(vmake_obj_instance *obj) {
  vmake_table_free(&obj->fields);
  free(obj);
}

vmake_obj_method *vmake_obj_method_new(vmake_state *state, const char *name,
                                       vmake_native_method method, int arity) {
  vmake_obj_method *obj = OBJ_NEW(vmake_obj_method, OBJ_METHOD);
  obj->method = method;
  obj->name = vmake_obj_string_new(state, (char *)name, strlen(name), true);
  obj->arity = arity;
  return obj;
}

void vmake_obj_method_free(vmake_obj_method *obj) {
  vmake_obj_string_free(obj->name);
  free(obj);
}

vmake_obj_table *vmake_obj_table_new(vmake_state *state, vmake_table table) {
  vmake_obj_table *obj = OBJ_NEW(vmake_obj_table, OBJ_TABLE);
  obj->table = malloc(sizeof(table));
  *obj->table = table;
  return obj;
}

void vmake_obj_table_free(vmake_obj_table *obj) {
  free(obj->table);
  free(obj);
}
