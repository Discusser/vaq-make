#pragma once

#include "value.h"
#include <stddef.h>

typedef struct vmake_variable vmake_variable;

typedef struct vmake_value_array {
  vmake_value *values;
  int size;
  int capacity;
} vmake_value_array;

typedef struct vmake_variable_array {
  vmake_variable *values;
  int size;
  int capacity;
} vmake_variable_array;

void vmake_value_array_new(vmake_value_array *arr);
void vmake_value_array_free(vmake_value_array *arr);
void vmake_value_array_push(vmake_value_array *arr, vmake_value val);
vmake_value vmake_value_array_pop(vmake_value_array *arr);

void vmake_variable_array_new(vmake_variable_array *arr);
void vmake_variable_array_free(vmake_variable_array *arr);
void vmake_variable_array_push(vmake_variable_array *arr, vmake_variable val);
vmake_variable vmake_variable_array_pop(vmake_variable_array *arr);
