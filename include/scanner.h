#pragma once

typedef enum vaq_make_token_type {
  TOKEN_EOF,
  TOKEN_ERROR,
  TOKEN_SEMICOLON,
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_EQUAL,
  TOKEN_EQUAL_EQUAL,
  TOKEN_NOT_EQUAL,
  TOKEN_LESS,
  TOKEN_LESS_EQUAL,
  TOKEN_GREATER,
  TOKEN_GREATER_EQUAL,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_STAR,
  TOKEN_SLASH,
  TOKEN_NOT,
  TOKEN_COMMA,
  TOKEN_STRING,
  TOKEN_NUMBER,
  TOKEN_IDENTIFIER,
  TOKEN_DOT,
  TOKEN_PRINT,
  TOKEN_FALSE,
  TOKEN_TRUE,
  TOKEN_NULL,
} vaq_make_token_type;

typedef struct vaq_make_token {
  // This pointer is not null-terminated
  const char *name;
  int name_length;
  int line;
  vaq_make_token_type type;
} vaq_make_token;

typedef struct vaq_make_scanner {
  // A pointer to the start of the current token
  const char *token_start;
  // A pointer to the current character we're reading
  const char *current_char;
  // The line we're currently on
  int line;
} vaq_make_scanner;

vaq_make_scanner vaq_make_init_scanner(const char *source);
vaq_make_token vaq_make_scan_token(vaq_make_scanner *scanner);
