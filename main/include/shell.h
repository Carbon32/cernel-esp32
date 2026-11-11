#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*

   There we go, Carblang now functions here. I will see how I can tinker around with it and make it more
   useful. Maybe add more fancy stuff, who knows.
    
*/

#define false 0
#define true 1

#define C_MAX_INPUT 128
#define C_MAX_ARGS 32
#define HISTORY_SIZE 5

extern char history[HISTORY_SIZE][C_MAX_INPUT];
extern int8_t history_counter;

void add_entry(const char *);
void process_command(const char *);
bool read_line(char *, size_t);
void shell_loop();

// This mix of C and C++ is too much sometimes
#ifdef __cplusplus
    extern "C" void shell_redraw_prompt(void);
    extern "C" void shell_notify(const char *message);
#else
    void shell_redraw_prompt(void);
    void shell_notify(const char *message);
#endif

void help_cmd();
void history_cmd();
void print_cmd(int8_t, char *[]);

void carblang_repl(void);

void start_script_server(void);
bool script_server_is_connected(void);
const char *script_server_ip(void);

bool carblang_wifi_set_ssid(const char *ssid);
bool carblang_wifi_set_password(const char *password);
bool carblang_wifi_connect(void);
bool carblang_wifi_disconnect(void);
void carblang_wifi_status(void);
bool carblang_wifi_scan(void);
