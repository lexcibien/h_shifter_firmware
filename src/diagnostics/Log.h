#pragma once

class Log {
public:
#if H_SHIFTER_LOG
  static void info(const char* message);
  static void event(const char* message);
  static void warning(const char* message);
#else
  static constexpr void info(const char* /*unused*/) {}
  static constexpr void event(const char* /*unused*/) {}
  static constexpr void warning(const char* /*unused*/) {}
#endif
};
