#include "shell.h"

void help_cmd() {
  printf("Available commands: \n");
  printf("help: \tshow available commands\n");
}

void history_cmd() {
  for (size_t i = 0; i < HISTORY_SIZE; i++) {
    if (history[i][0] == '\0') {
      printf("- (empty)\n");
    } else {
      printf("- %s\n", history[i]);
    }
  }
}

void print_cmd(int8_t argc, char *argv[]) {
  for (size_t i = 1; i < argc; i++) {
    printf("%s", argv[i]);

    if (i < argc - 1) {
      printf(" ");
    }
  }
  printf("\n");
}