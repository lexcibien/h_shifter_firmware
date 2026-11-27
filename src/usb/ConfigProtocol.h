#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

class ConfigProtocol {
public:
  static constexpr uint8_t VERSION = 1;
  static constexpr size_t FRAME_SIZE = 64;
  static constexpr size_t PAYLOAD_SIZE = 52;

  enum class Command : uint8_t {
    GET_INFO = 0x01,
    GET_CONFIG = 0x02,
    SAVE_CONFIG = 0x03,
    RESTORE_DEFAULTS = 0x04,
    GET_STATUS = 0x05,
    REDETECT_HANDLE = 0x06,
    REBOOT = 0x07,
  };

  enum class Status : uint8_t {
    OK = 0,
    BAD_FRAME = 1,
    BAD_VERSION = 2,
    UNSUPPORTED = 3,
    LOCKED = 4,
    INVALID_CONFIG = 5,
    FLASH_ERROR = 6,
    BUSY = 7,
  };

  class Handler {
  public:
    virtual ~Handler() = default;
    virtual Status handleConfigCommand(Command command, std::span<const uint8_t> request, std::span<uint8_t> response, uint8_t& responseSize) = 0;
  };

  explicit ConfigProtocol(Handler& handler) : handler_(&handler) {}
  [[nodiscard]] std::span<uint8_t> incoming();
  void receive(size_t count);
  [[nodiscard]] std::span<const uint8_t> outgoing() const;
  void transmit(size_t count);
  void reset();

  static uint32_t crc32(std::span<const uint8_t> bytes);

private:
  void discardStreamPrefix();
  [[nodiscard]] bool hasValidCrc() const;
  void processFrame();
  void prepareResponse(uint8_t command, uint16_t transaction, Status status, std::span<const uint8_t> payload = {});

  Handler* handler_;
  std::array<uint8_t, FRAME_SIZE> receiveBuffer_{};
  std::array<uint8_t, FRAME_SIZE> responseBuffer_{};
  size_t received_ = 0;
  size_t transmitted_ = FRAME_SIZE;
};
