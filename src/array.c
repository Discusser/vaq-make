#include "array.h"
#include "generator.h"
#include <stdlib.h>

void vmake_value_array_new(vmake_value_array *arr) {
  arr->values = NULL;
  arr->capacity = 0;
  arr->size = 0;
}

void vmake_value_array_free(vmake_value_array *arr) {
  free(arr->values);
  vmake_value_array_new(arr);
}

void vmake_value_array_push(vmake_value_array *arr, vmake_value val) {
  if (arr->size + 1 > arr->capacity) {
    arr->capacity = arr->capacity < 8 ? 8 : arr->capacity * 2;
    arr->values = reallocarray(arr->values, arr->capacity, sizeof(vmake_value));
  }

  arr->values[arr->size++] = val;
}

vmake_value vmake_value_array_pop(vmake_value_array *arr) { return arr->values[--arr->size]; }

void vmake_variable_array_new(vmake_variable_array *arr) {
  arr->values = NULL;
  arr->capacity = 0;
  arr->size = 0;
}

void vmake_variable_array_free(vmake_variable_array *arr) {
  free(arr->values);
  vmake_variable_array_new(arr);
}

void vmake_variable_array_push(vmake_variable_array *arr, vmake_variable val) {
  if (arr->size + 1 > arr->capacity) {
    arr->capacity = arr->capacity < 8 ? 8 : arr->capacity * 2;
    arr->values = reallocarray(arr->values, arr->capacity, sizeof(vmake_variable));
  }

  arr->values[arr->size++] = val;
}

vmake_variable vmake_variable_array_pop(vmake_variable_array *arr) {
  return arr->values[--arr->size];
}
