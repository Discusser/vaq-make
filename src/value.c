#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

vaq_make_value vaq_make_value_number(double number) {
  return (vaq_make_value){VAL_NUMBER, {.number = number}};
}

vaq_make_value vaq_make_value_bool(bool boolean) {
  return (vaq_make_value){VAL_BOOL, {.boolean = boolean}};
}

vaq_make_value vaq_make_value_nil() { return (vaq_make_value){VAL_NIL, {.number = 0}}; }

vaq_make_value vaq_make_value_obj(vaq_make_obj *obj) {
  return (vaq_make_value){VAL_OBJ, {.obj = obj}};
}

char *vaq_make_value_to_string(vaq_make_value val) {
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
  case VAL_OBJ:
    buf = vaq_make_obj_to_string(val.as.obj);
  }

  return buf;
}

void vaq_make_value_print(vaq_make_value val) {
  char *buf = vaq_make_value_to_string(val);
  printf("%s", buf);
  free(buf);
}

bool vaq_make_value_equals(vaq_make_value a, vaq_make_value b) {
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
    break;
  }
}

int vaq_make_value_compare(vaq_make_value a, vaq_make_value b) {
  switch (a.type) {
  case VAL_NUMBER: {
    double x = a.as.number;
    double y = b.as.number;
    return x == y ? 0 : x > y ? 1 : -1;
  }
  case VAL_BOOL:
  case VAL_NIL:
  case VAL_OBJ:
    return 0;
    break;
  }
}
