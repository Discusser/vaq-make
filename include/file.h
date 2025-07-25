#pragma once

// Equivalent to realpath(path, NULL);
char *vmake_path_abs(const char *path);
// Resolves a file path relative to another file.
// For example, vmake_path_rel("path/to/file.txt", "../other/dir/file2.txt") returns
// "path/other/dir/file2.txt"
char *vmake_path_rel(const char *current, const char *relative);
