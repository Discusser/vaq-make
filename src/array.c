#include "array.h"
#include "generator.h"
#include <stdlib.h>

void vaq_make_value_array_new(vaq_make_value_array *arr) {
  arr->values = NULL;
  arr->capacity = 0;
  arr->size = 0;
}

void vaq_make_value_array_free(vaq_make_value_array *arr) {
  free(arr->values);
  vaq_make_value_array_new(arr);
}

void vaq_make_value_array_push(vaq_make_value_array *arr, vaq_make_value val) {
  if (arr->size + 1 > arr->capacity) {
    arr->capacity = arr->capacity < 8 ? 8 : arr->capacity * 2;
    arr->values = reallocarray(arr->values, arr->capacity, sizeof(vaq_make_value));
  }

  arr->values[arr->size++] = val;
}

vaq_make_value vaq_make_value_array_pop(vaq_make_value_array *arr) {
  return arr->values[--arr->size];
}

void vaq_make_variable_array_new(vaq_make_variable_array *arr) {
  arr->values = NULL;
  arr->capacity = 0;
  arr->size = 0;
}

void vaq_make_variable_array_free(vaq_make_variable_array *arr) {
  free(arr->values);
  vaq_make_variable_array_new(arr);
}

void vaq_make_variable_array_push(vaq_make_variable_array *arr, vaq_make_variable val) {
  if (arr->size + 1 > arr->capacity) {
    arr->capacity = arr->capacity < 8 ? 8 : arr->capacity * 2;
    arr->values = reallocarray(arr->values, arr->capacity, sizeof(vaq_make_variable));
  }

  arr->values[arr->size++] = val;
}

vaq_make_variable vaq_make_variable_array_pop(vaq_make_variable_array *arr) {
  return arr->values[--arr->size];
}
