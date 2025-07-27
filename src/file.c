#include "file.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *vmake_path_abs(const char *path) { return realpath(path, NULL); }

char *vmake_path_rel(const char *current, const char *relative) {
  // If relative is an absolute path we can just directly return it.
  if (*relative == '/')
    return strdup(relative);

  int current_len = strlen(current);
  char *current_copy = malloc(sizeof(char) * (current_len + 2));
  strcpy(current_copy, current);
  char *end = strrchr(current_copy, '/');
  // If we found a last '/' character, and current is not only a '/', then end the path right after
  if (end != NULL && current_len > 0) {
    *(end + 1) = '\0';
  } else if (end == NULL) { // If there is no '/' character, the path is a file name, therefore we
                            // can discard it
    *current_copy = '\0';
  }
  current_copy = realloc(current_copy, sizeof(char) * (current_len + strlen(relative) + 2));
  strcat(current_copy, relative);
  char *real = realpath(current_copy, NULL);
  free(current_copy);
  return real;
}

char *vmake_path_abs_to_rel(const char *abs) {
  char *cwd = getcwd(NULL, 0);
  int cwd_len = strlen(cwd);
  // The absolute path is in the current working directory
  if (strncmp(cwd, abs, cwd_len) == 0) {
    int abs_len = strlen(abs);
    int res_len = abs_len - cwd_len + strlen(".");
    char *res = malloc(sizeof(char) * (res_len + 1));
    if (sprintf(res, ".%s", abs + cwd_len) != res_len) {
      vmake_error_exit(NULL, CTX_INTERNAL, NULL, "An error occurred while writing to a string.");
    }

    free(cwd);
    return res;
  } else {
    // The absolute path is not in the current working directory, so we have to find the last common
    // directory.
    int previous_dir_index = 0;
    int abs_len = strlen(abs);
    for (int i = 0; i < abs_len; i++) {
      if (abs[i] != cwd[i])
        break;
      if (abs[i] == '/')
        previous_dir_index = i;
    }

    int dir_count = 1;
    for (const char *p = abs + previous_dir_index + 1; *p != '\0'; p++) {
      if (*p == '/')
        dir_count++;
    }

    int res_len = dir_count * strlen("../") + abs_len - previous_dir_index;
    char *res = malloc(sizeof(char) * (res_len + 1));
    int pos = 0;
    for (int i = 0; i < dir_count; i++) {
      memcpy(res + pos, "../", 3);
      pos += 3;
    }
    strcat(res, abs + previous_dir_index + 1);

    free(cwd);
    return res;
  }
}
