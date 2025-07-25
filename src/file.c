#include "file.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *vmake_path_abs(const char *path) { return realpath(path, NULL); }

char *vmake_path_rel(const char *current, const char *relative) {
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
