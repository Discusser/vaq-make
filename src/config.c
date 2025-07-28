#include "config.h"
#include "file.h"
#include <libgen.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static vmake_makefile create_file_for_target(vmake_state *state, char *name);
static vmake_makefile build_target(vmake_state *state, vmake_value target);
static vmake_makefile build_executable(vmake_state *state, vmake_obj_instance *inst);

#define NULL_MAKEFILE (vmake_makefile){NULL, NULL, NULL};

void vmake_build_makefiles(vmake_state *state, char *build_directory, char *source_directory) {
  state->make.build_directory = build_directory;
  state->make.source_directory = source_directory;

  vmake_create_directory(state->make.source_directory);
  vmake_create_directory(state->make.build_directory);

  vmake_makefile main;
  main.dir_path = state->make.build_directory;
  main.name = "Makefile";
  char *path;
  asprintf(&path, "%s/Makefile", state->make.build_directory);
  main.fp = fopen(path, "w");
  free(path);

  char self_path[PATH_MAX];
  int self_path_len = readlink("/proc/self/exe", self_path, PATH_MAX);
  if (self_path_len == -1)
    vmake_error_exit(NULL, CTX_INTERNAL, NULL,
                     "An error occurred while trying to read /proc/self/exe.");
  self_path[self_path_len] = '\0';
  fprintf(main.fp, "VMAKE = %s\n", self_path);
  fprintf(main.fp, "VMAKE_FILE = %s\n", state->root_file);
  fprintf(main.fp, "VMAKE_ARGS =");
  for (int i = 1; i < state->argc; i++) {
    fprintf(main.fp, " %s", state->argv[i]);
  }
  fprintf(main.fp, "\n\n");
  fprintf(main.fp, "default_target: self\n");
  fprintf(main.fp, ".PHONY: default_target\n\n");
  vmake_string_buf target_rules;
  vmake_string_buf_new(&target_rules);
  for (int i = 0; i < state->make.targets.size; i++) {
    vmake_makefile target = build_target(state, state->make.targets.values[i]);
    if (target.fp != NULL) {
      fprintf(main.fp, "%s:\n", target.name);
      fprintf(main.fp, "\t$(MAKE) -s -f %s/build.make %s\n", target.dir_path, target.name);
      fclose(target.fp);
      fprintf(main.fp, ".PHONY: %s\n\n", target.name);

      vmake_string_buf_append(&target_rules, " %s", target.name);
    }
  }
  fprintf(main.fp, "all:%s\n", target_rules.string);
  fprintf(main.fp, ".PHONY: all\n\n");
  vmake_string_buf_free(&target_rules);
  fprintf(main.fp, "self: $(VMAKE_FILE)\n");
  fprintf(main.fp, "\t$(VMAKE) $(VMAKE_ARGS)\n");
  fprintf(main.fp, "\t$(MAKE) -s -f Makefile all\n");
  fprintf(main.fp, ".PHONY: self\n");

  fclose(main.fp);
}

static vmake_makefile create_file_for_target(vmake_state *state, char *name) {
  vmake_makefile makefile;

  int dir_path_len = strlen(state->make.build_directory) + strlen("/target.") + strlen(name);

  makefile.dir_path = malloc(sizeof(char) * (dir_path_len + 1));
  // NOTE: We probably shouldn't pass name directly, as it could contain illegal characters for
  // paths.
  sprintf(makefile.dir_path, "%s/target.%s", state->make.build_directory, name);
  vmake_create_directory(makefile.dir_path);

  char *file_path = malloc(sizeof(char) * (dir_path_len + strlen("/build.make") + 1));
  sprintf(file_path, "%s/build.make", makefile.dir_path);
  makefile.fp = vmake_new_makefile(file_path);
  makefile.name = name;

  return makefile;
}

static vmake_makefile build_target(vmake_state *state, vmake_value target) {
  if (target.type == VAL_OBJ) {
    switch (target.as.obj->type) {
    case OBJ_INSTANCE: {
      vmake_obj_instance *inst = ((vmake_obj_instance *)target.as.obj);
      if (inst->klass == state->classes[CLASS_EXECUTABLE]) {
        return build_executable(state, inst);
      } else {
        char *class_name = vmake_obj_to_string((vmake_obj *)inst->klass->name);
        vmake_error(NULL, CTX_INTERNAL, NULL, "Tried building target of unknown type %s.",
                    class_name);
        free(class_name);
      }
      return NULL_MAKEFILE;
    }
    default:
      vmake_error(NULL, CTX_INTERNAL, NULL, "Tried building target of unknown type %s.",
                  vmake_obj_type_to_string(target.as.obj->type));
      return NULL_MAKEFILE;
    }
  } else {
    vmake_error(NULL, CTX_INTERNAL, NULL, "Tried building target of unknown type %s.",
                vmake_value_type_to_string(target.type));
    return NULL_MAKEFILE;
  }
}

static vmake_makefile build_executable(vmake_state *state, vmake_obj_instance *inst) {
  char *name =
      ((vmake_obj_string *)vmake_obj_instance_get_field(inst, state, "name").as.obj)->chars;
  vmake_makefile file = create_file_for_target(state, name);

  vmake_value_array *sources =
      ((vmake_obj_array *)vmake_obj_instance_get_field(inst, state, "sources").as.obj)->array;

  {
    vmake_value val = vmake_obj_instance_get_field(inst, state, "include_directories");
    if (val.type != VAL_NIL) {
      vmake_value_array *inc_dirs = ((vmake_obj_array *)val.as.obj)->array;
      for (int i = 0; i < inc_dirs->size; i++) {
        if (!vmake_value_is_string(sources->values[i]))
          vmake_error_exit(NULL, CTX_INTERNAL, NULL,
                           "Expected string in executable include directories.");
        fprintf(file.fp, "CFLAGS += -I%s\n",
                ((vmake_obj_string *)inc_dirs->values[i].as.obj)->chars);
      }
      if (inc_dirs->size > 0)
        fprintf(file.fp, "\n");
    }
  }

  {
    vmake_value val = vmake_obj_instance_get_field(inst, state, "link_libraries");
    if (val.type != VAL_NIL) {
      vmake_value_array *libs = ((vmake_obj_array *)val.as.obj)->array;
      for (int i = 0; i < libs->size; i++) {
        if (!vmake_value_is_string(sources->values[i]))
          vmake_error_exit(NULL, CTX_INTERNAL, NULL,
                           "Expected string in executable link libraries.");
        fprintf(file.fp, "LIBS += -l%s\n", ((vmake_obj_string *)libs->values[i].as.obj)->chars);
      }
      if (libs->size > 0)
        fprintf(file.fp, "\n");
    }
  }

  // Write dependencies for target
  int objects_capacity = 8;
  int objects_len = 0;
  char **objects = malloc(sizeof(char *) * objects_capacity);
  for (int i = 0; i < sources->size; i++) {
    if (!vmake_value_is_string(sources->values[i]))
      vmake_error_exit(NULL, CTX_INTERNAL, NULL, "Expected string in executable sources.");
    char *source_str = ((vmake_obj_string *)sources->values[i].as.obj)->chars;
    char *source_path_rel = vmake_path_rel(state->make.source_directory, source_str);
    char *source_path = realpath(source_path_rel, NULL);
    free(source_path_rel);
    if (source_path == NULL)
      vmake_error_exit(NULL, CTX_INTERNAL, NULL, "Couldn't find file with path '%s'", source_str);
    int source_dir_len = strlen(state->make.source_directory);
    if (strncmp(state->make.source_directory, source_path, source_dir_len) != 0) {
      vmake_error_exit(NULL, CTX_USER, NULL, "Source file at '%s' is not in source directory '%s'",
                       source_path, state->make.source_directory);
    }
    int source_path_len = strlen(source_path);
    if (source_path[source_path_len - 2] == '.' && source_path[source_path_len - 1] == 'c') {
      source_path[source_path_len - 1] = 'o';
    }
    char *object_path;
    asprintf(&object_path, "%s/objects/%s", state->make.build_directory,
             source_path + source_dir_len + 1);
    free(source_path);
    fprintf(file.fp, "%s: %s\n", name, object_path);

    if (objects_len >= objects_capacity) {
      objects_capacity *= 2;
      objects = realloc(objects, sizeof(char *) * objects_capacity);
    }
    objects[objects_len++] = object_path;

    char *object_copy = strdup(object_path);
    // Ensure the directory exists so that Make doesn't throw any errors
    vmake_create_directory(dirname(object_copy));
    free(object_copy);
  }
  fprintf(file.fp, "%s:\n", name);
  fprintf(file.fp, "\t$(CC) $(CFLAGS) $^ -o $@ $(LIBS)\n\n");

  int loop_end = sources->size <= objects_len ? sources->size : objects_len;
  for (int i = 0; i < loop_end; i++) {
    char *source_str = ((vmake_obj_string *)sources->values[i].as.obj)->chars;
    fprintf(file.fp, "%s: %s\n", objects[i], source_str);
    fprintf(file.fp, "\t$(CC) -c $(CFLAGS) $^ -o $@\n");
  }
  fprintf(file.fp, "\n");

  return file;
}
