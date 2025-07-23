#include "scanner-priv.h"
#include <ctype.h>
#include <stdio.h>
#include "scanner.h"

vaq_make_scanner vaq_make_init_scanner(const char *source) {
  vaq_make_scanner scanner;

  scanner.current_char = source;
  scanner.token_start = source;
  scanner.line = 0;

  return scanner;
}

vaq_make_token vaq_make_scan_token(vaq_make_scanner *scanner) {
  consume_ignored(scanner);

  scanner->token_start = scanner->current_char;

  if (is_eof(scanner))
    return make_token(scanner, TOKEN_EOF);

  char c = consume(scanner);

  if (c == '\n')
    scanner->line++;

  if (isdigit(c)) {
    return make_number(scanner);
  } else if (is_identifier_char(c, true)) {
    return make_identifier(scanner);
  }

  switch (c) {
  case '=':
    return match(scanner, '=') ? make_token(scanner, TOKEN_EQUAL_EQUAL)
                               : make_token(scanner, TOKEN_EQUAL);
  case ';':
    return make_token(scanner, TOKEN_SEMICOLON);
  case '(':
    return make_token(scanner, TOKEN_LEFT_PAREN);
  case ')':
    return make_token(scanner, TOKEN_RIGHT_PAREN);
  case '.':
    return make_token(scanner, TOKEN_DOT);
  case '"':
    return make_string(scanner);
  }

  return make_token(scanner, TOKEN_ERROR);
}

void consume_ignored(vaq_make_scanner *scanner) {
  while (true) {
    char c = peek(scanner);
    switch (c) {
    case '\n':
      scanner->line++;
      consume(scanner);
      break;
    case ' ':
    case '\t':
    case '\r':
      consume(scanner);
      break;
    case '#':
      while (peek(scanner) != '\n' && is_eof(scanner))
        consume(scanner);
      break;
    default:
      return;
    }
  }
}

char consume(vaq_make_scanner *scanner) { return *scanner->current_char++; }

char peek(vaq_make_scanner *scanner) { return *scanner->current_char; }

bool is_eof(vaq_make_scanner *scanner) { return peek(scanner) == '\0'; }

bool match(vaq_make_scanner *scanner, char c) {
  if (peek(scanner) == c) {
    scanner->current_char++;
    return true;
  }

  return false;
}

bool is_identifier_char(char c, bool first) {
  return c == '_' || (first ? isalpha(c) : isalnum(c));
}

vaq_make_token make_number(vaq_make_scanner *scanner) {
  while (isdigit(peek(scanner))) {
    consume(scanner);
  }

  return make_token(scanner, TOKEN_NUMBER);
}

vaq_make_token make_identifier(vaq_make_scanner *scanner) {
  while (is_identifier_char(peek(scanner), false)) {
    consume(scanner);
  }

  return make_token(scanner, TOKEN_IDENTIFIER);
}

vaq_make_token make_string(vaq_make_scanner *scanner) {
  char c;
  do {
    c = consume(scanner);
  } while (c != '"' && c != '\0');

  // Get rid of the " at the beginning.
  scanner->token_start++;
  vaq_make_token token = make_token(scanner, TOKEN_STRING);
  // Get rid of the " at the end.
  token.name_length--;
  return token;
}

vaq_make_token make_token(vaq_make_scanner *scanner, vaq_make_token_type type) {
  vaq_make_token token;
  token.type = type;
  token.line = scanner->line;
  token.name = scanner->token_start;
  token.name_length = scanner->current_char - scanner->token_start;
  return token;
}
