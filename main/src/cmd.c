#include "shell.h"

void help_cmd() {
  printf("Available commands: \n");
  printf("help: \tshow available commands\n");
  printf("print: \tdisplay some text\n");
  printf("version: \tdipslay cernel version\n");
  printf("carblang: \tstart the Carblang interactive prompt\n");
  printf("wifi status: \tshow WiFi state and connected network\n");
  printf("wifi scan: \tlist nearby WiFi networks\n");
  printf("wifi set ssid <name>: \tset WiFi SSID\n");
  printf("wifi set password <password>: \tset WiFi password\n");
  printf("wifi connect: \tconnect to the configured network\n");
  printf("wifi disconnect: \tdisconnect from WiFi\n");
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
