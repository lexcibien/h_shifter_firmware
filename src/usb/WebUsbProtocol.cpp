#include "WebUsbProtocol.h"
#include <algorithm>
#include <class/vendor/vendor_device.h>
#include <cstring>

namespace {
constexpr size_t PAYLOAD_OFFSET = 8;
constexpr size_t CRC_OFFSET = 60;

uint16_t readU16(std::span<const uint8_t> bytes, size_t offset) {
  return static_cast<uint16_t>(bytes[offset]) | (static_cast<uint16_t>(bytes[offset + 1]) << 8U);
}

void writeU16(std::span<uint8_t> bytes, size_t offset, uint16_t value) {
  bytes[offset] = static_cast<uint8_t>(value);
  bytes[offset + 1] = static_cast<uint8_t>(value >> 8U);
}

uint32_t readU32(std::span<const uint8_t> bytes, size_t offset) {
  uint32_t value = 0;
  for (size_t index = 0; index < sizeof(value); ++index) {
    value |= static_cast<uint32_t>(bytes[offset + index]) << (index * 8U);
  }
  return value;
}

void writeU32(std::span<uint8_t> bytes, size_t offset, uint32_t value) {
  for (size_t index = 0; index < sizeof(value); ++index) {
    bytes[offset + index] = static_cast<uint8_t>(value >> (index * 8U));
  }
}
} // namespace

uint32_t WebUsbProtocol::crc32(std::span<const uint8_t> bytes) {
  uint32_t crc = 0xFFFFFFFF;
  for (const uint8_t byte : bytes) {
    crc ^= byte;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1U) ^ (0xEDB88320U & (0U - (crc & 1U)));
    }
  }
  return ~crc;
}

void WebUsbProtocol::reset() {
  received_ = 0;
  responsePending_ = false;
}

void WebUsbProtocol::update() {
  if (responsePending_) {
    if (tud_vendor_write_available() < FRAME_SIZE) {
      return;
    }
    if (tud_vendor_write(responseBuffer_.data(), responseBuffer_.size()) == responseBuffer_.size()) {
      tud_vendor_write_flush();
      responsePending_ = false;
    }
    return;
  }

  while (received_ < FRAME_SIZE && tud_vendor_available() > 0) {
    const uint32_t count = tud_vendor_read(receiveBuffer_.data() + received_, FRAME_SIZE - received_);
    if (count == 0) {
      break;
    }
    received_ += count;
  }
  if (received_ == FRAME_SIZE) {
    processFrame();
    received_ = 0;
  }
}

void WebUsbProtocol::processFrame() {
  const std::span<const uint8_t> frame(receiveBuffer_);
  const uint8_t command = frame[3];
  const uint16_t transaction = readU16(frame, 4);
  if (frame[0] != 'H' || frame[1] != 'S' || frame[6] > PAYLOAD_SIZE || frame[7] != 0 ||
      readU32(frame, CRC_OFFSET) != crc32(frame.first(CRC_OFFSET))) {
    prepareResponse(command, transaction, Status::BAD_FRAME);
    return;
  }
  if (frame[2] != VERSION) {
    prepareResponse(command, transaction, Status::BAD_VERSION);
    return;
  }

  std::array<uint8_t, PAYLOAD_SIZE - 1> response{};
  uint8_t responseSize = 0;
  const auto status = handler_.handleWebUsbCommand(static_cast<Command>(command), frame.subspan(PAYLOAD_OFFSET, frame[6]), response, responseSize);
  if (responseSize > response.size()) {
    prepareResponse(command, transaction, Status::BAD_FRAME);
    return;
  }
  prepareResponse(command, transaction, status, std::span<const uint8_t>(response).first(responseSize));
}

void WebUsbProtocol::prepareResponse(uint8_t command, uint16_t transaction, Status status, std::span<const uint8_t> payload) {
  responseBuffer_.fill(0);
  responseBuffer_[0] = 'H';
  responseBuffer_[1] = 'S';
  responseBuffer_[2] = VERSION;
  responseBuffer_[3] = command | 0x80U;
  writeU16(responseBuffer_, 4, transaction);
  responseBuffer_[6] = static_cast<uint8_t>(payload.size() + 1);
  responseBuffer_[PAYLOAD_OFFSET] = static_cast<uint8_t>(status);
  std::copy(payload.begin(), payload.end(), responseBuffer_.begin() + PAYLOAD_OFFSET + 1);
  writeU32(responseBuffer_, CRC_OFFSET, crc32(std::span<const uint8_t>(responseBuffer_).first(CRC_OFFSET)));
  responsePending_ = true;
}
