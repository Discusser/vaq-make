#pragma once

#include "value.h"
#include <stddef.h>

typedef struct vaq_make_variable vaq_make_variable;

typedef struct vaq_make_value_array {
  vaq_make_value *values;
  int size;
  int capacity;
} vaq_make_value_array;

typedef struct vaq_make_variable_array {
  vaq_make_variable *values;
  int size;
  int capacity;
} vaq_make_variable_array;

void vaq_make_value_array_new(vaq_make_value_array *arr);
void vaq_make_value_array_free(vaq_make_value_array *arr);
void vaq_make_value_array_push(vaq_make_value_array *arr, vaq_make_value val);
vaq_make_value vaq_make_value_array_pop(vaq_make_value_array *arr);

void vaq_make_variable_array_new(vaq_make_variable_array *arr);
void vaq_make_variable_array_free(vaq_make_variable_array *arr);
void vaq_make_variable_array_push(vaq_make_variable_array *arr, vaq_make_variable val);
vaq_make_variable vaq_make_variable_array_pop(vaq_make_variable_array *arr);
