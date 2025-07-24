#pragma once

typedef enum vaq_make_obj_type {
  OBJ_STRING,
} vaq_make_obj_type;

typedef struct vaq_make_obj {
  vaq_make_obj_type type;
} vaq_make_obj;
