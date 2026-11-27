#include "UsbConfiguration.h"
#include <tusb.h>

namespace {

template <typename Read, typename Write> void service(ConfigProtocol& protocol, Read read, Write write) {
  const auto outgoing = protocol.outgoing();
  if (!outgoing.empty()) {
    protocol.transmit(write(outgoing));
    return;
  }
  protocol.receive(read(protocol.incoming()));
}

#if H_SHIFTER_CONFIG_CDC
// Pico stdio owns CDC instance zero in log builds. TinyUSB invokes the callback
// while polling the USB task, before this loop services the protocol.
constexpr uint8_t CONFIG_CDC_INSTANCE = H_SHIFTER_LOG;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool serialResetPending = false;
#endif

} // namespace

#if H_SHIFTER_CONFIG_CDC
extern "C" void tud_cdc_line_state_cb(uint8_t instance, bool dtr, bool /*rts*/) {
  if (instance == CONFIG_CDC_INSTANCE && !dtr) {
    serialResetPending = true;
    tud_cdc_n_read_flush(instance);
    tud_cdc_n_write_clear(instance);
  }
}
#endif

UsbConfiguration::UsbConfiguration(ConfigProtocol::Handler& handler)
    : web_(handler, ConfigProtocol::Framing::ALIGNED)
#if H_SHIFTER_CONFIG_CDC
      ,
      serial_(handler, ConfigProtocol::Framing::STREAM)
#endif
{
}

void UsbConfiguration::update() {
  service(
      web_, [](std::span<uint8_t> data) { return tud_vendor_read(data.data(), data.size()); },
      [](std::span<const uint8_t> data) -> size_t {
        if (tud_vendor_write_available() < data.size()) {
          return 0;
        }
        const size_t count = tud_vendor_write(data.data(), data.size());
        tud_vendor_write_flush();
        return count;
      });

#if H_SHIFTER_CONFIG_CDC
  if (serialResetPending) {
    serial_.reset();
    serialResetPending = false;
  }
  if (tud_cdc_n_connected(CONFIG_CDC_INSTANCE)) {
    service(
        serial_, [](std::span<uint8_t> data) { return tud_cdc_n_read(CONFIG_CDC_INSTANCE, data.data(), data.size()); },
        [](std::span<const uint8_t> data) -> size_t {
          if (tud_cdc_n_write_available(CONFIG_CDC_INSTANCE) < data.size()) {
            return 0;
          }
          const size_t count = tud_cdc_n_write(CONFIG_CDC_INSTANCE, data.data(), data.size());
          tud_cdc_n_write_flush(CONFIG_CDC_INSTANCE);
          return count;
        });
  }
#endif
}

void UsbConfiguration::reset() {
  web_.reset();
#if H_SHIFTER_CONFIG_CDC
  serial_.reset();
  serialResetPending = false;
#endif
}
