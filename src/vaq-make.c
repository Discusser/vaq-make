#include <stdio.h>
#include <stdlib.h>
#include "generator-priv.h"
#include "scanner.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s [vmake_file]\n", argv[0]);
    exit(1);
  }

  FILE *file = fopen(argv[1], "r");
  if (!file) {
    fprintf(stderr, "An error occurred while trying to open file at '%s'.\n",
            argv[1]);
  }
  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);
  char *source = malloc(sizeof(char) * (file_size + 1));
  if (source == NULL) {
    fprintf(stderr, "Could not allocate buffer to store file at \"%s\"\n",
            argv[1]);
    exit(1);
  }
  size_t bytes_read = fread(source, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read file at '%s'\n", argv[1]);
    exit(1);
  }
  source[bytes_read] = '\0';
  fclose(file);

  vaq_make_scanner scanner = vaq_make_init_scanner(source);
  vaq_make_token token;
  do {
    token = vaq_make_scan_token(&scanner);
    printf("LINE %i : Token '%.*s' type %i\n", token.line, token.name_length,
           token.name, token.type);
  } while (token.type != TOKEN_EOF);
  vaq_make_generate_build();

  return 0;
}
