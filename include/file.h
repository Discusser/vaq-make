#pragma once

#include <stdio.h>

// Equivalent to realpath(path, NULL);
char *vmake_path_abs(const char *path);
// Resolves a file path relative to another file.
// For example, vmake_path_rel("path/to/file.txt", "../other/dir/file2.txt") returns
// "path/other/dir/file2.txt"
char *vmake_path_rel(const char *current, const char *relative);
// Returns the absolute path `abs` relative to the current working directory. `abs` is expected to
// be a file path, not a directory path.
char *vmake_path_abs_to_rel(const char *abs);

void vmake_create_directory(const char *path);
FILE *vmake_new_makefile(const char *path);
