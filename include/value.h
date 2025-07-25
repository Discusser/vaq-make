#pragma once

#include "object.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum vmake_value_type {
  VAL_EMPTY,
  VAL_NUMBER,
  VAL_BOOL,
  VAL_NIL,
  VAL_OBJ
} vmake_value_type;

typedef struct vmake_obj vmake_obj;

typedef struct vmake_value {
  vmake_value_type type;
  union {
    double number;
    bool boolean;
    vmake_obj *obj;
  } as;
} vmake_value;

vmake_value vmake_value_empty();
vmake_value vmake_value_number(double number);
vmake_value vmake_value_bool(bool boolean);
vmake_value vmake_value_nil();
vmake_value vmake_value_obj(vmake_obj *obj);

bool vmake_value_is_obj(vmake_value val);
bool vmake_value_is_string(vmake_value val);
bool vmake_value_is_native(vmake_value val);

uint32_t vmake_value_hash(vmake_value val);

char *vmake_value_to_string(vmake_value val);
void vmake_value_print(vmake_value val);
bool vmake_value_equals(vmake_value a, vmake_value b);
int vmake_value_compare(vmake_value a, vmake_value b);
