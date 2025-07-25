#include "generator.h"
#include "scanner-priv.h"
#include "scanner.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s [vmake_file]\n", argv[0]);
    exit(1);
  }

  char *source = read_file(argv[1], NULL);
  vmake_scanner scanner = vmake_init_scanner(source);
  vmake_generate_build(&scanner, argv[1]);
  free(source);

  return 0;
}
