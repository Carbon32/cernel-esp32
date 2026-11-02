#include "shell.h"
#include "led.h"

char history[HISTORY_SIZE][C_MAX_INPUT] = {0};
int8_t history_counter = 0;

void add_entry(const char *input) {
  strncpy(history[history_counter], input, C_MAX_INPUT - 1);
  history[history_counter][C_MAX_INPUT - 1] = '\0';

  history_counter = (history_counter + 1) % HISTORY_SIZE;
}

void process_command(const char *input) {
  if (input == NULL || input[0] == '\0') {
    return;
  }

  add_entry(input);

  char buffer[C_MAX_INPUT];

  strncpy(buffer, input, C_MAX_INPUT - 1);
  buffer[C_MAX_INPUT - 1] = '\0';

  char *argv[C_MAX_ARGS];
  int argc = 0;

  char *token = strtok(buffer, " \t\r\n");

  while (token != NULL && argc < C_MAX_ARGS - 1) {
    argv[argc++] = token;
    token = strtok(NULL, " \t\r\n");
  }

  argv[argc] = NULL;

  if (argc == 0) {
    return;
  }

  for (size_t i = 0; argv[0][i]; i++) {
    argv[0][i] = tolower((unsigned char)argv[0][i]);
  }

  char *command = argv[0];

  if (strcmp(command, "help") == 0) {
    help_cmd();
  }

  else if (strcmp(command, "history") == 0) {
    history_cmd();
  }

  else if (strcmp(command, "print") == 0) {
    print_cmd(argc, argv);
  }

  else if (strcmp(command, "version") == 0) {
    printf("Cernel v0.0.1\n");
  }

  else if (strcmp(command, "led") == 0) {
    printf("GPIO2 LED switched.\nIf you have no built-in LED, attach a LED to "
           "GPIO2.\n");
    switch_native_led();
  }

  else if (strcmp(command, "wifi") == 0) {
    printf("To be implemented\n");
  }

  else if (strcmp(command, "bluetooth") == 0) {
    printf("To be implemented\n");
  }

  else if (strcmp(command, "cpu") == 0) {
    printf("To be implemented\n");
  }

  else if (strcmp(command, "mem") == 0) {
    printf("To be implemented\n");
  }

  else if (strcmp(command, "uptime") == 0) {
    printf("To be implemented\n");
  }

  else if (strcmp(command, "reboot") == 0) {
    printf("To be implemented\n");
  }

  else {
    printf("Unknown command: %s\n", command);
    printf("Type \"help\" to see available commands.\n");
  }
}

bool read_line(char *buffer, size_t size) {
  static bool skip_lf = false;
  size_t length = 0;

  while (1) {
    int ch = getchar();
    if (ch == EOF) {
      return false;
    }

    if (skip_lf && ch == '\n') {
      skip_lf = false;
      continue;
    }
    skip_lf = false;

    if (ch == '\r' || ch == '\n') {
      if (ch == '\r') {
        skip_lf = true;
      }
      buffer[length] = '\0';
      printf("\r\n");
      return true;
    }

    if ((ch == '\b' || ch == 127) && length > 0) {
      length--;
      printf("\b \b");
    } else if (ch >= 32 && ch <= 126 && length < size - 1) {
      buffer[length++] = (char)ch;
      putchar(ch);
    }

    fflush(stdout);
  }
}

void shell_loop() {
  char input[C_MAX_INPUT];
  while (1) {
    printf("cernel@cernel> ");
    fflush(stdout);
    if (!read_line(input, sizeof(input))) {
      printf("Input read failed\n");
      continue;
    }

    process_command(input);
  }
}