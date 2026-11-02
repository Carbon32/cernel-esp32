#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*

    This shell will be turned into something more advanced, something that uses some sort
    of a programming language to get things done but I'm still unsure of how I am going
    to do it, for now, I will stick to keeping everything simple
    
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

void help_cmd();
void history_cmd();
void print_cmd(int8_t, char *[]);