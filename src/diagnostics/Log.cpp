#if H_SHIFTER_LOG

#include "Log.h"
#include <Adafruit_USBD_CDC.h>

static void writeLog(const char* level, const char* message) {
  Serial.print(level);
  Serial.println(message);
}

void Log::info(const char* message) { writeLog("[INFO] ", message); }

void Log::event(const char* message) { writeLog("[EVENT] ", message); }

void Log::warning(const char* message) { writeLog("[WARN] ", message); }

#endif
