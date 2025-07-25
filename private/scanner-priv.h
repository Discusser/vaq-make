#pragma once

#include "scanner.h"
#include <stdbool.h>
#include <stddef.h>

char *read_file(const char *path, size_t *file_size);

bool is_identifier_char(char c, bool first);

vmake_token make_number(vmake_scanner *scanner);
vmake_token make_identifier(vmake_scanner *scanner);
vmake_token make_string(vmake_scanner *scanner);
vmake_token_type identifier_type(vmake_scanner *scanner);
vmake_token_type check_keyword(vmake_scanner *scanner, int start, int length, const char *rest,
                               vmake_token_type type);

vmake_token make_token(vmake_scanner *scanner, vmake_token_type type);
