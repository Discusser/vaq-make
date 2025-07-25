#include "common.h"
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

  vmake_state state;
  vmake_table_init(&state.globals);
  vmake_table_init(&state.strings);
  vmake_value_array_new(&state.include_stack);

  char *path = realpath(argv[1], NULL);
  vmake_process_path(&state, path);
  free(path);

  vmake_value_array_free(&state.include_stack);
  vmake_table_free(&state.strings);
  vmake_table_free(&state.globals);

  return 0;
}

void vmake_process_path(vmake_state *state, char *path) {
  char *source = read_file(path, NULL);
  vmake_scanner scanner = vmake_init_scanner(source);
  vmake_generate_build(&scanner, state, path);
  free(source);
}
