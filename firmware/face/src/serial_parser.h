// firmware/face/src/serial_parser.h
// Reads complete \n-terminated lines from Serial and dispatches them.
#pragma once
#include <Arduino.h>

// Callback signature: receives a parsed command line (no trailing \n).
typedef void (*CommandHandler)(const char *line);

void parser_setup(CommandHandler handler);
void parser_poll();  // call every loop iteration
