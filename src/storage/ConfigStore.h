#pragma once

#include "config/DeviceConfig.h"
#include <cstdint>

class ConfigStore {
public:
  static constexpr uint32_t RECORD_SIZE = 256;
  enum class Source : uint8_t { DEFAULTS = 0, FLASH = 1 };

  struct LoadResult {
    DeviceConfig config = DeviceConfig::defaults();
    uint32_t generation = 0;
    Source source = Source::DEFAULTS;
  };

  LoadResult load();
  bool save(const DeviceConfig& config);

  [[nodiscard]] uint32_t generation() const { return generation_; }
  [[nodiscard]] Source source() const { return source_; }

private:
  static constexpr uint32_t SLOT_SIZE = 4096;
  static constexpr uint32_t RESERVED_SIZE = SLOT_SIZE * 2;
  uint32_t generation_ = 0;
  int8_t activeSlot_ = -1;
  Source source_ = Source::DEFAULTS;
};
