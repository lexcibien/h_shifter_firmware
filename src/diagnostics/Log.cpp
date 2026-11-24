#if H_SHIFTER_LOG

#include "Log.h"
#include <cstdio>
#include <pico/stdio.h>

static void writeLog(const char* level, const char* message) {
  stdio_put_string(level, -1, false, false);
  puts(message);
}

void Log::info(const char* message) { writeLog("[INFO] ", message); }

void Log::event(const char* message) { writeLog("[EVENT] ", message); }

void Log::warning(const char* message) { writeLog("[WARN] ", message); }

#endif
