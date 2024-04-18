#include <stdlib.h>

#define NOB_IMPL
#include "nob.h"

void cc(NOB_Cmd *cmd) {
  nob_cmd_append(cmd, "cc");
  nob_cmd_append(cmd, "-Wall", "-Wextra", "-ggdb");
  nob_cmd_append(cmd, "-O3");
}

int main(int argc, char **argv) {
  GO_REBUILD_YOURSELF(argc, argv);

  const char *program = nob_shift_args(&argc, &argv);
  (void)program;

  NOB_Cmd cmd = {0};

  if (!nob_mkdir_if_not_exists("./build/"))
    return EXIT_FAILURE;

  const char *main_input = "bf-jit.c";
  const char *main_output = "./build/bf-jit";

  cc(&cmd);
  nob_cmd_append(&cmd, "-o", main_output);
  nob_cmd_append(&cmd, main_input);

  if (!nob_cmd_run_sync(cmd))
    return EXIT_FAILURE;

  cmd.count = 0;
  nob_cmd_append(&cmd, main_output);
  nob_da_append_many(&cmd, argv, argc);
  if (!nob_cmd_run_sync(cmd))
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
