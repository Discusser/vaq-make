#pragma once

typedef enum vmake_token_type {
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
  TOKEN_INCLUDE,
} vmake_token_type;

typedef struct vmake_token {
  // This pointer is not null-terminated
  const char *name;
  int name_length;
  int line;
  vmake_token_type type;
} vmake_token;

typedef struct vmake_scanner {
  // A pointer to the start of the current token
  const char *token_start;
  // A pointer to the current character we're reading
  const char *current_char;
  // The line we're currently on
  int line;
} vmake_scanner;

vmake_scanner vmake_init_scanner(const char *source);
vmake_token vmake_scan_token(vmake_scanner *scanner);
