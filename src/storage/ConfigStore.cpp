#include "ConfigStore.h"
#include "usb/WebUsbProtocol.h"
#include <array>
#include <cstring>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <pico/flash.h>
#include <pico/platform.h>

namespace {
constexpr uint32_t RECORD_MAGIC = 0x43465348; // "HSFC" in little-endian.
constexpr size_t HEADER_AND_PAYLOAD_SIZE = 12 + DeviceConfig::WIRE_SIZE;
constexpr size_t CRC_OFFSET = HEADER_AND_PAYLOAD_SIZE;
constexpr uint32_t SLOT_SIZE = 4096;
constexpr uint32_t SLOT_OFFSETS[] = { PICO_FLASH_SIZE_BYTES, PICO_FLASH_SIZE_BYTES + SLOT_SIZE };

void writeU16(std::span<uint8_t> bytes, size_t offset, uint16_t value) {
  bytes[offset] = static_cast<uint8_t>(value);
  bytes[offset + 1] = static_cast<uint8_t>(value >> 8U);
}

void writeU32(std::span<uint8_t> bytes, size_t offset, uint32_t value) {
  for (size_t index = 0; index < sizeof(value); ++index) {
    bytes[offset + index] = static_cast<uint8_t>(value >> (index * 8U));
  }
}

uint16_t readU16(std::span<const uint8_t> bytes, size_t offset) {
  return static_cast<uint16_t>(bytes[offset]) | (static_cast<uint16_t>(bytes[offset + 1]) << 8U);
}

uint32_t readU32(std::span<const uint8_t> bytes, size_t offset) {
  uint32_t value = 0;
  for (size_t index = 0; index < sizeof(value); ++index) {
    value |= static_cast<uint32_t>(bytes[offset + index]) << (index * 8U);
  }
  return value;
}

struct SlotRecord {
  bool valid = false;
  uint32_t generation = 0;
  DeviceConfig config{};
};

SlotRecord readSlot(uint8_t slot) {
  const auto* address = reinterpret_cast<const uint8_t*>(XIP_BASE + SLOT_OFFSETS[slot]);
  const std::span<const uint8_t> record(address, ConfigStore::RECORD_SIZE);
  SlotRecord result;
  if (readU32(record, 0) != RECORD_MAGIC || readU16(record, 4) != DeviceConfig::SCHEMA_VERSION || record[6] != SHIFTER_PROJECT_VERSION ||
      record[7] != DeviceConfig::WIRE_SIZE) {
    return result;
  }
  if (readU32(record, CRC_OFFSET) != WebUsbProtocol::crc32(record.first(CRC_OFFSET))) {
    return result;
  }
  if (!DeviceConfig::decode(record.subspan(12, DeviceConfig::WIRE_SIZE), result.config)) {
    return result;
  }
  result.generation = readU32(record, 8);
  result.valid = true;
  return result;
}

struct FlashOperation {
  uint32_t offset;
  const uint8_t* record;
};

void __not_in_flash_func(programSlot)(void* parameter) {
  const auto* operation = static_cast<const FlashOperation*>(parameter);
  flash_range_erase(operation->offset, FLASH_SECTOR_SIZE);
  flash_range_program(operation->offset, operation->record, FLASH_PAGE_SIZE);
}
} // namespace

ConfigStore::LoadResult ConfigStore::load() {
  const SlotRecord first = readSlot(0);
  const SlotRecord second = readSlot(1);
  if (!first.valid && !second.valid) {
    generation_ = 0;
    activeSlot_ = -1;
    source_ = Source::DEFAULTS;
    return {};
  }

  const bool useSecond = second.valid && (!first.valid || static_cast<int32_t>(second.generation - first.generation) > 0);
  const SlotRecord& selected = useSecond ? second : first;
  activeSlot_ = useSecond ? 1 : 0;
  generation_ = selected.generation;
  source_ = Source::FLASH;
  return { .config = selected.config, .generation = selected.generation, .source = Source::FLASH };
}

bool ConfigStore::save(const DeviceConfig& config) {
  if (!config.isValid()) {
    return false;
  }

  std::array<uint8_t, RECORD_SIZE> record{};
  record.fill(0xFF);
  writeU32(record, 0, RECORD_MAGIC);
  writeU16(record, 4, DeviceConfig::SCHEMA_VERSION);
  record[6] = SHIFTER_PROJECT_VERSION;
  record[7] = DeviceConfig::WIRE_SIZE;
  const uint32_t nextGeneration = generation_ + 1;
  writeU32(record, 8, nextGeneration);
  const auto payload = config.encode();
  std::memcpy(record.data() + 12, payload.data(), payload.size());
  writeU32(record, CRC_OFFSET, WebUsbProtocol::crc32(std::span<const uint8_t>(record).first(CRC_OFFSET)));

  const uint8_t targetSlot = activeSlot_ == 0 ? 1 : 0;
  FlashOperation operation{ .offset = SLOT_OFFSETS[targetSlot], .record = record.data() };
  if (flash_safe_execute(programSlot, &operation, 1000) != PICO_OK) {
    return false;
  }

  const SlotRecord verification = readSlot(targetSlot);
  if (!verification.valid || verification.generation != nextGeneration || verification.config != config) {
    return false;
  }
  activeSlot_ = static_cast<int8_t>(targetSlot);
  generation_ = nextGeneration;
  source_ = Source::FLASH;
  return true;
}
