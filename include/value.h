#pragma once

#include "object.h"
#include <stdbool.h>

typedef enum vaq_make_value_type { VAL_NUMBER, VAL_BOOL, VAL_NIL, VAL_OBJ } vaq_make_value_type;

typedef struct vaq_make_value {
  vaq_make_value_type type;
  union {
    double number;
    bool boolean;
    vaq_make_obj *obj;
  } as;
} vaq_make_value;

vaq_make_value vaq_make_value_number(double number);
vaq_make_value vaq_make_value_bool(bool boolean);
vaq_make_value vaq_make_value_nil();
vaq_make_value vaq_make_value_obj(vaq_make_obj *obj);

char *vaq_make_value_to_string(vaq_make_value val);
void vaq_make_value_print(vaq_make_value val);
bool vaq_make_value_equals(vaq_make_value a, vaq_make_value b);
int vaq_make_value_compare(vaq_make_value a, vaq_make_value b);
