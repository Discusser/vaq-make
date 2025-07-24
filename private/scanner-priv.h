#pragma once

#include "scanner.h"
#include <stdbool.h>
#include <stddef.h>

char *read_file(const char *path, size_t *file_size);

bool is_identifier_char(char c, bool first);

vaq_make_token make_number(vaq_make_scanner *scanner);
vaq_make_token make_identifier(vaq_make_scanner *scanner);
vaq_make_token make_string(vaq_make_scanner *scanner);
vaq_make_token_type identifier_type(vaq_make_scanner *scanner);
vaq_make_token_type check_keyword(vaq_make_scanner *scanner, int start, int length,
                                  const char *rest, vaq_make_token_type type);

vaq_make_token make_token(vaq_make_scanner *scanner, vaq_make_token_type type);
