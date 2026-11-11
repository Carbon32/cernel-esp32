#include "shell.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "led.h"

char history[HISTORY_SIZE][C_MAX_INPUT] = {0};
int8_t history_counter = 0;

void add_entry(const char *input) {
  strncpy(history[history_counter], input, C_MAX_INPUT - 1);
  history[history_counter][C_MAX_INPUT - 1] = '\0';

  history_counter = (history_counter + 1) % HISTORY_SIZE;
}

static const char *skip_spaces(const char *cursor) {
  while (*cursor == ' ' || *cursor == '\t') {
    cursor++;
  }
  return cursor;
}

static bool consume_word(const char **cursor, const char *word) {
  const size_t length = strlen(word);
  if (strncmp(*cursor, word, length) != 0 ||
      ((*cursor)[length] != '\0' && (*cursor)[length] != ' ' &&
       (*cursor)[length] != '\t')) {
    return false;
  }
  *cursor = skip_spaces(*cursor + length);
  return true;
}

static bool extract_wifi_value(const char *input, const char *field,
                               char *value, size_t capacity) {
  const char *cursor = skip_spaces(input);
  if (!consume_word(&cursor, "wifi") || !consume_word(&cursor, "set") ||
      !consume_word(&cursor, field)) {
    return false;
  }

  const char *start = cursor;
  const char *end = NULL;
  if (*start == '"') {
    start++;
    end = strchr(start, '"');
    if (end == NULL || *skip_spaces(end + 1) != '\0') {
      return false;
    }
  } else {
    end = start + strlen(start);
    while (end > start && (end[-1] == ' ' || end[-1] == '\t')) {
      end--;
    }
  }

  const size_t length = (size_t)(end - start);
  if (length >= capacity) {
    return false;
  }
  memcpy(value, start, length);
  value[length] = '\0';
  return true;
}

void process_command(const char *input) {
  if (input == NULL || input[0] == '\0') {
    return;
  }

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

  for (int i = 0; i < argc && i < 3; i++) {
    for (size_t j = 0; argv[i][j]; j++) {
      argv[i][j] = tolower((unsigned char)argv[i][j]);
    }
  }

  if (strcmp(argv[0], "wifi") == 0 && argc >= 3 &&
      strcmp(argv[1], "set") == 0 && strcmp(argv[2], "password") == 0) {
    add_entry("wifi set password <hidden>");
  } else {
    add_entry(input);
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
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "status") == 0)) {
      carblang_wifi_status();
    }

    else if (argc >= 3 && strcmp(argv[1], "set") == 0 &&
             strcmp(argv[2], "ssid") == 0) {
      char ssid[33];
      if (!extract_wifi_value(input, "ssid", ssid, sizeof(ssid)) ||
          !carblang_wifi_set_ssid(ssid)) {
        printf("Could not set SSID. Use: wifi set ssid <name>\n");
      }

      else {
        printf("WiFi SSID set in RAM. It will be cleared on reboot.\n");
      }
    }

    else if (argc >= 3 && strcmp(argv[1], "set") == 0 &&
             strcmp(argv[2], "password") == 0) {
      char password[65];
      if (!extract_wifi_value(input, "password", password, sizeof(password)) ||
          !carblang_wifi_set_password(password)) {
        printf("Could not set password. Use: wifi set password <password>\n");
      }

      else {
        printf("WiFi password set in RAM. It will be cleared on reboot.\n");
      }
    }

    else if (argc == 2 && strcmp(argv[1], "scan") == 0) {
      carblang_wifi_scan();
    }

    else if (argc == 2 && strcmp(argv[1], "connect") == 0) {
      if (carblang_wifi_connect()) {
        printf("Connecting to WiFi; use \"wifi status\" to check the state.\n");
      }

      else {
        printf("Could not start WiFi. Set the SSID and password first.\n");
      }
    }

    else if (argc == 2 && strcmp(argv[1], "disconnect") == 0) {
      if (carblang_wifi_disconnect()) {
        printf("WiFi disconnected. Credentials remain in RAM until reboot.\n");
      }

      else {
        printf("Could not disconnect WiFi.\n");
      }
    }

    else {
      printf("Use: wifi status | wifi scan | wifi set ssid <name> | "
             "wifi set password <password> | wifi connect | wifi disconnect\n");
    }
  }

  else if (strcmp(command, "carblang") == 0) {
    carblang_repl();
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
    uint8_t input_byte;
    if (uart_read_bytes(UART_NUM_0, &input_byte, 1, portMAX_DELAY) != 1) {
      return false;
    }
    int ch = input_byte;

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
      uart_write_bytes(UART_NUM_0, "\r\n", 2);
      return true;
    }

    if ((ch == '\b' || ch == 127) && length > 0) {
      length--;
      uart_write_bytes(UART_NUM_0, "\b \b", 3);
    } else if (ch >= 32 && ch <= 126 && length < size - 1) {
      buffer[length++] = (char)ch;
      const char echo = (char)ch;
      uart_write_bytes(UART_NUM_0, &echo, 1);
    }
  }
}

void shell_loop() {
  char input[C_MAX_INPUT];
  while (1) {
    const char prompt[] = "cernel@cernel> ";
    uart_write_bytes(UART_NUM_0, prompt, sizeof(prompt) - 1);
    if (!read_line(input, sizeof(input))) {
      printf("Input read failed\n");
      continue;
    }

    process_command(input);
  }
}

void shell_redraw_prompt(void) {
  uart_write_bytes(UART_NUM_0, "\r\n", 2);
  const char prompt[] = "cernel@cernel> ";
  uart_write_bytes(UART_NUM_0, prompt, sizeof(prompt) - 1);
}

void shell_notify(const char *message) {
  if (message == NULL) {
    return;
  }

  char output[160];
  const int length =
      snprintf(output, sizeof(output), "\r\n%s\r\ncernel@cernel> ", message);

  if (length > 0) {
    const size_t output_size =
        (size_t)length < sizeof(output) ? (size_t)length : sizeof(output) - 1;
    uart_write_bytes(UART_NUM_0, output, output_size);
  }
}
