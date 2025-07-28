#include "file.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *vmake_path_abs(const char *path) { return realpath(path, NULL); }

char *vmake_path_rel(const char *current, const char *relative) {
  // If relative is an absolute path we can just directly return it.
  if (*relative == '/')
    return strdup(relative);

  int current_to_copy = 0;
  bool add_slash = false;
  int relative_to_copy = 0;

  struct stat path_stat;
  stat(current, &path_stat);
  if (S_ISDIR(path_stat.st_mode)) {
    // If current is a directory, we want to copy the whole string, plus a trailing slash if there
    // is none
    current_to_copy = strlen(current);
    if (current[current_to_copy - 1] != '/')
      add_slash = true;
  } else {
    // If current is a file path, we want to discard the filename
    // We just want to copy the directory name followed by a slash
    current_to_copy = strrchr(current, '/') - current;
    add_slash = true;
  }

  int len = current_to_copy + strlen(relative);
  if (add_slash)
    len++;
  char *out = malloc(sizeof(char) * (len + 1));
  if (add_slash)
    sprintf(out, "%.*s/%s", current_to_copy, current, relative);
  else
    sprintf(out, "%.*s%s", current_to_copy, current, relative);

  char *real = realpath(out, NULL);
  free(out);

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

void vmake_create_directory(const char *path) {
  int path_len = strlen(path);
  for (int i = 0; i < path_len; i++) {
    if (path[i] == '/' || path[i + 1] == '\0') {
      // PERF: This can be optimized by using realloc instead of allocating and freeing each time,
      // or by just using a char[PATH_MAX]. I don't think this will be too much of an issue in terms
      // of performance especially since there's file IO involved.
      int size = path[i + 1] == '\0' ? i + 1 : i;
      char *subdir = malloc(sizeof(char) * (size + 1));
      subdir[0] = '\0';
      strncat(subdir, path, size);
      mkdir(subdir, S_IRWXU);
      free(subdir);
    }
  }
}

FILE *vmake_new_makefile(const char *path) {
  FILE *fp = fopen(path, "w");

  return fp;
}
