// firmware/face/src/serial_parser.cpp
#include "serial_parser.h"

static char buf[128];
static uint8_t buf_pos = 0;
static CommandHandler s_handler = nullptr;

void parser_setup(CommandHandler handler) {
    s_handler = handler;
    buf_pos = 0;
}

void parser_poll() {
    while (Serial.available()) {
        int c = Serial.read();
        if (c < 0) break;
        if (c == '\n') {
            buf[buf_pos] = 0;
            if (buf_pos > 0 && s_handler) {
                s_handler(buf);
            }
            buf_pos = 0;
        } else if (c == '\r') {
            // ignore
        } else if (buf_pos < sizeof(buf) - 1) {
            buf[buf_pos++] = (char)c;
        } else {
            // overflow — reset and log
            Serial.println("LOG buf_overflow");
            buf_pos = 0;
        }
    }
}
