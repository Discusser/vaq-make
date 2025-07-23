#pragma once

#include "scanner.h"
#include <stddef.h>
#include <stdbool.h>

void consume_ignored(vaq_make_scanner *scanner);
char consume(vaq_make_scanner *scanner);
char peek(vaq_make_scanner *scanner);
bool is_eof(vaq_make_scanner *scanner);
bool match(vaq_make_scanner *scanner, char c);

bool is_identifier_char(char c, bool first);

vaq_make_token make_number(vaq_make_scanner *scanner);
vaq_make_token make_identifier(vaq_make_scanner *scanner);
vaq_make_token make_string(vaq_make_scanner *scanner);

vaq_make_token make_token(vaq_make_scanner *scanner, vaq_make_token_type type);
