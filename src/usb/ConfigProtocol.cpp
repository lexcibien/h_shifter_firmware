#include "ConfigProtocol.h"
#include <algorithm>
#include <cstring>

constexpr size_t PAYLOAD_OFFSET = 8;
constexpr size_t CRC_OFFSET = 60;
constexpr uint8_t MAGIC_FIRST = 'H';
constexpr uint8_t MAGIC_SECOND = 'S';

static uint16_t readU16(std::span<const uint8_t> bytes, size_t offset) {
  return static_cast<uint16_t>(bytes[offset]) | static_cast<uint16_t>(static_cast<uint16_t>(bytes[offset + 1]) << 8U);
}

static void writeU16(std::span<uint8_t> bytes, size_t offset, uint16_t value) {
  bytes[offset] = static_cast<uint8_t>(value);
  bytes[offset + 1] = static_cast<uint8_t>(value >> 8U);
}

static uint32_t readU32(std::span<const uint8_t> bytes, size_t offset) {
  uint32_t value = 0;
  for (size_t index = 0; index < sizeof(value); ++index) {
    value |= static_cast<uint32_t>(bytes[offset + index]) << (index * 8U);
  }
  return value;
}

static void writeU32(std::span<uint8_t> bytes, size_t offset, uint32_t value) {
  for (size_t index = 0; index < sizeof(value); ++index) {
    bytes[offset + index] = static_cast<uint8_t>(value >> (index * 8U));
  }
}

uint32_t ConfigProtocol::crc32(std::span<const uint8_t> bytes) {
  uint32_t crc = 0xFFFFFFFF;
  for (const uint8_t byte : bytes) {
    crc ^= byte;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1U) ^ (0xEDB88320U & (0U - (crc & 1U)));
    }
  }
  return ~crc;
}

void ConfigProtocol::reset() {
  received_ = 0;
  transmitted_ = FRAME_SIZE;
}

std::span<uint8_t> ConfigProtocol::incoming() { return std::span(receiveBuffer_).subspan(received_); }

std::span<const uint8_t> ConfigProtocol::outgoing() const { return std::span(responseBuffer_).subspan(transmitted_); }

void ConfigProtocol::transmit(size_t count) { transmitted_ += count; }

bool ConfigProtocol::hasValidCrc() const {
  const std::span<const uint8_t> frame(receiveBuffer_);
  return readU32(frame, CRC_OFFSET) == crc32(frame.first(CRC_OFFSET));
}

void ConfigProtocol::discardStreamPrefix() {
  const auto bytes = std::span(receiveBuffer_).first(received_);
  for (size_t i = 1; i + 1 < bytes.size(); ++i) {
    if (bytes[i] == MAGIC_FIRST && bytes[i + 1] == MAGIC_SECOND) {
      received_ -= i;
      std::memmove(receiveBuffer_.data(), receiveBuffer_.data() + i, received_);
      return;
    }
  }
  const bool trailingMagic = bytes.back() == MAGIC_FIRST;
  received_ = trailingMagic ? 1 : 0;
  if (trailingMagic) {
    receiveBuffer_[0] = MAGIC_FIRST;
  }
}

void ConfigProtocol::receive(size_t count) {
  received_ += count;
  if (received_ < FRAME_SIZE) {
    return;
  }
  // Only a CRC-checked candidate can establish the next CDC frame boundary.
  if (receiveBuffer_[0] != MAGIC_FIRST || receiveBuffer_[1] != MAGIC_SECOND || !hasValidCrc()) {
    discardStreamPrefix();
    return;
  }
  processFrame();
  received_ = 0;
}

void ConfigProtocol::processFrame() {
  const std::span<const uint8_t> frame(receiveBuffer_);
  const uint8_t command = frame[3];
  const uint16_t transaction = readU16(frame, 4);
  if (frame[6] > PAYLOAD_SIZE || frame[7] != 0) {
    prepareResponse(command, transaction, Status::BAD_FRAME);
    return;
  }
  if (frame[2] != VERSION) {
    prepareResponse(command, transaction, Status::BAD_VERSION);
    return;
  }

  std::array<uint8_t, PAYLOAD_SIZE - 1> response{};
  uint8_t responseSize = 0;
  const auto status = handler_->handleConfigCommand(static_cast<Command>(command), frame.subspan(PAYLOAD_OFFSET, frame[6]), response, responseSize);
  if (responseSize > response.size()) {
    prepareResponse(command, transaction, Status::BAD_FRAME);
    return;
  }
  prepareResponse(command, transaction, status, std::span<const uint8_t>(response).first(responseSize));
}

void ConfigProtocol::prepareResponse(uint8_t command, uint16_t transaction, Status status, std::span<const uint8_t> payload) {
  responseBuffer_.fill(0);
  responseBuffer_.at(0) = MAGIC_FIRST;
  responseBuffer_.at(1) = MAGIC_SECOND;
  responseBuffer_.at(2) = VERSION;
  responseBuffer_.at(3) = command | 0x80U;
  writeU16(responseBuffer_, 4, transaction);
  responseBuffer_.at(6) = static_cast<uint8_t>(payload.size() + 1);
  responseBuffer_.at(PAYLOAD_OFFSET) = static_cast<uint8_t>(status);
  std::ranges::copy(payload, responseBuffer_.begin() + PAYLOAD_OFFSET + 1);
  writeU32(responseBuffer_, CRC_OFFSET, crc32(std::span<const uint8_t>(responseBuffer_).first(CRC_OFFSET)));
  transmitted_ = 0;
}
