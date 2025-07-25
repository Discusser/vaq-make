#include "value.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

vmake_value vmake_value_empty() { return (vmake_value){VAL_EMPTY, {.number = 0}}; }

vmake_value vmake_value_number(double number) {
  return (vmake_value){VAL_NUMBER, {.number = number}};
}

vmake_value vmake_value_bool(bool boolean) { return (vmake_value){VAL_BOOL, {.boolean = boolean}}; }

vmake_value vmake_value_nil() { return (vmake_value){VAL_NIL, {.number = 0}}; }

vmake_value vmake_value_obj(vmake_obj *obj) { return (vmake_value){VAL_OBJ, {.obj = obj}}; }

bool vmake_value_is_obj(vmake_value val) { return val.type == VAL_OBJ; }

bool vmake_value_is_string(vmake_value val) {
  return vmake_value_is_obj(val) && val.as.obj->type == OBJ_STRING;
}

bool vmake_value_is_native(vmake_value val) {
  return vmake_value_is_obj(val) && val.as.obj->type == OBJ_NATIVE;
}

bool vmake_value_is_array(vmake_value val) {
  return vmake_value_is_obj(val) && val.as.obj->type == OBJ_ARRAY;
}

uint32_t vmake_value_hash(vmake_value val) {
  switch (val.type) {
  case VAL_BOOL:
    return val.as.boolean;
  case VAL_NIL:
    return 3;
  case VAL_NUMBER: {
    union bitcast {
      double value;
      uint32_t ints[2];
    };

    union bitcast cast;
    cast.value = (val.as.number) + 1.0;
    return cast.ints[0] + cast.ints[1];
  }
  case VAL_OBJ:
    if (val.as.obj->type == OBJ_STRING)
      return ((vmake_obj_string *)val.as.obj)->hash;
    return (uint32_t)(intptr_t)val.as.obj;
  default:
    return 0;
  }
}

char *vmake_value_to_string(vmake_value val) {
  char *buf;
  switch (val.type) {
  case VAL_NUMBER: {
    int len;
    if ((len = asprintf(&buf, "%f", val.as.number)) == -1) {
      fprintf(stderr, "An internal error occurred while trying to print a number.");
    }
    bool found_dot = false;
    int previous_digit_index = -1;
    for (int i = 0; i < len; i++) {
      if (buf[i] == '.') {
        found_dot = true;
        previous_digit_index = i - 1;
      }
      if (found_dot) {
        if ('1' <= buf[i] && buf[i] <= '9')
          previous_digit_index = i;
      }
    }
    if (found_dot)
      len = previous_digit_index + 1;
    buf[len] = '\0';
    break;
  }
  case VAL_BOOL:
    buf = malloc(sizeof(char) * ((val.as.boolean ? 4 : 5) + 1));
    strcpy(buf, val.as.boolean ? "true" : "false");
    break;
  case VAL_NIL:
    buf = malloc(sizeof(char) * 5);
    strcpy(buf, "null");
    break;
  case VAL_EMPTY:
    buf = malloc(sizeof(char) * 6);
    strcpy(buf, "empty");
    break;
  case VAL_OBJ:
    buf = vmake_obj_to_string(val.as.obj);
  }

  return buf;
}

void vmake_value_print(vmake_value val) {
  char *buf = vmake_value_to_string(val);
  printf("%s", buf);
  free(buf);
}

bool vmake_value_equals(vmake_value a, vmake_value b) {
  if (a.type != b.type)
    return false;

  switch (a.type) {
  case VAL_NUMBER:
    return a.as.number == b.as.number;
  case VAL_BOOL:
    return a.as.boolean == b.as.boolean;
  case VAL_NIL:
    return true;
  case VAL_OBJ:
    return a.as.obj == b.as.obj;
  case VAL_EMPTY:
    return false;
  }

  return false;
}

int vmake_value_compare(vmake_value a, vmake_value b) {
  switch (a.type) {
  case VAL_NUMBER: {
    double x = a.as.number;
    double y = b.as.number;
    return x == y ? 0 : x > y ? 1 : -1;
  }
  case VAL_BOOL:
  case VAL_NIL:
  case VAL_OBJ:
  case VAL_EMPTY:
    return 0;
    break;
  }
}
