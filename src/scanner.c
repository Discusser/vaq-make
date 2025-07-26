#include "scanner.h"
#include "scanner-priv.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void consume_ignored(vmake_scanner *scanner);
static char consume(vmake_scanner *scanner);
static char peek(vmake_scanner *scanner);
static bool is_eof(vmake_scanner *scanner);
static bool match(vmake_scanner *scanner, char c);

char *read_file(const char *path, size_t *file_size) {
  FILE *file = fopen(path, "r");
  if (!file) {
    fprintf(stderr,
            "An error occurred while trying to open file at '%s'. The file either doesn't exist or "
            "requires elevated permissions.\n",
            path);
    exit(1);
  }
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);
  char *source = malloc(sizeof(char) * (size + 1));
  if (source == NULL) {
    fprintf(stderr, "Could not allocate buffer to store file at \"%s\"\n", path);
    exit(1);
  }
  size_t bytes_read = fread(source, sizeof(char), size, file);
  if (bytes_read < size) {
    fprintf(stderr, "Could not read file at '%s'\n", path);
    exit(1);
  }
  source[bytes_read] = '\0';
  fclose(file);

  if (file_size != NULL)
    *file_size = size;
  return source;
}

vmake_scanner vmake_init_scanner(const char *source) {
  vmake_scanner scanner;

  scanner.current_char = source;
  scanner.token_start = source;
  scanner.line = 0;

  return scanner;
}

vmake_token vmake_scan_token(vmake_scanner *scanner) {
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
    return make_token(scanner, match(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case ';':
    return make_token(scanner, TOKEN_SEMICOLON);
  case '(':
    return make_token(scanner, TOKEN_LEFT_PAREN);
  case ')':
    return make_token(scanner, TOKEN_RIGHT_PAREN);
  case '.':
    return make_token(scanner, TOKEN_DOT);
  case '!':
    return make_token(scanner, match(scanner, '=') ? TOKEN_NOT_EQUAL : TOKEN_NOT);
  case '<':
    return make_token(scanner, match(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '>':
    return make_token(scanner, match(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '+':
    return make_token(scanner, TOKEN_PLUS);
  case '-':
    return make_token(scanner, TOKEN_MINUS);
  case '*':
    return make_token(scanner, TOKEN_STAR);
  case '/':
    return make_token(scanner, TOKEN_SLASH);
  case ',':
    return make_token(scanner, TOKEN_COMMA);
  case '[':
    return make_token(scanner, TOKEN_LEFT_SQUARE_BRACKET);
  case ']':
    return make_token(scanner, TOKEN_RIGHT_SQUARE_BRACKET);
  case '"':
    return make_string(scanner);
  }

  return make_token(scanner, TOKEN_ERROR);
}

static void consume_ignored(vmake_scanner *scanner) {
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
      while (peek(scanner) != '\n' && !is_eof(scanner))
        consume(scanner);
      break;
    default:
      return;
    }
  }
}

static char consume(vmake_scanner *scanner) { return *scanner->current_char++; }

static char peek(vmake_scanner *scanner) { return *scanner->current_char; }

static bool is_eof(vmake_scanner *scanner) { return peek(scanner) == '\0'; }

static bool match(vmake_scanner *scanner, char c) {
  if (peek(scanner) == c) {
    scanner->current_char++;
    return true;
  }

  return false;
}

bool is_identifier_char(char c, bool first) {
  return c == '_' || (first ? isalpha(c) : isalnum(c));
}

vmake_token make_number(vmake_scanner *scanner) {
  while (isdigit(peek(scanner))) {
    consume(scanner);
  }

  return make_token(scanner, TOKEN_NUMBER);
}

vmake_token make_identifier(vmake_scanner *scanner) {
  while (is_identifier_char(peek(scanner), false)) {
    consume(scanner);
  }

  return make_token(scanner, identifier_type(scanner));
}

vmake_token make_string(vmake_scanner *scanner) {
  char c;
  do {
    c = consume(scanner);
  } while (c != '"' && c != '\0');

  // Get rid of the " at the beginning.
  scanner->token_start++;
  vmake_token token = make_token(scanner, TOKEN_STRING);
  // Get rid of the " at the end.
  token.name_length--;
  return token;
}

vmake_token_type identifier_type(vmake_scanner *scanner) {
  switch (*scanner->token_start) {
  case 'f':
    return check_keyword(scanner, 1, 4, "alse", TOKEN_FALSE);
  case 'i':
    return check_keyword(scanner, 1, 6, "nclude", TOKEN_INCLUDE);
  case 'n':
    return check_keyword(scanner, 1, 2, "il", TOKEN_NIL);
  case 'p':
    return check_keyword(scanner, 1, 4, "rint", TOKEN_PRINT);
  case 't':
    return check_keyword(scanner, 1, 3, "rue", TOKEN_TRUE);
  }

  return TOKEN_IDENTIFIER;
}

vmake_token_type check_keyword(vmake_scanner *scanner, int start, int length, const char *rest,
                               vmake_token_type type) {
  if (scanner->current_char - scanner->token_start == start + length &&
      memcmp(scanner->token_start + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

vmake_token make_token(vmake_scanner *scanner, vmake_token_type type) {
  vmake_token token;
  token.type = type;
  token.line = scanner->line;
  token.name = scanner->token_start;
  token.name_length = scanner->current_char - scanner->token_start;
  return token;
}
