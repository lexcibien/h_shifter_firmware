#include "UsbConfiguration.h"
#include <tusb.h>

namespace {

// Pico stdio owns CDC instance zero in log builds. TinyUSB invokes the callback
// while polling the USB task, before this loop services the protocol.
constexpr uint8_t CONFIG_CDC_INSTANCE = H_SHIFTER_LOG;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool serialResetPending = false;

} // namespace

extern "C" void tud_cdc_line_state_cb(uint8_t instance, bool dtr, bool /*rts*/) {
  if (instance == CONFIG_CDC_INSTANCE && !dtr) {
    serialResetPending = true;
    tud_cdc_n_read_flush(instance);
    tud_cdc_n_write_clear(instance);
  }
}

UsbConfiguration::UsbConfiguration(ConfigProtocol::Handler& handler) : protocol_(handler) {}

void UsbConfiguration::update() {
  if (serialResetPending) {
    reset();
  }
  if (!tud_cdc_n_connected(CONFIG_CDC_INSTANCE)) {
    return;
  }

  const auto outgoing = protocol_.outgoing();
  if (outgoing.empty()) {
    const auto incoming = protocol_.incoming();
    protocol_.receive(tud_cdc_n_read(CONFIG_CDC_INSTANCE, incoming.data(), incoming.size()));
  } else if (tud_cdc_n_write_available(CONFIG_CDC_INSTANCE) >= outgoing.size()) {
    protocol_.transmit(tud_cdc_n_write(CONFIG_CDC_INSTANCE, outgoing.data(), outgoing.size()));
    tud_cdc_n_write_flush(CONFIG_CDC_INSTANCE);
  }
}

void UsbConfiguration::reset() {
  protocol_.reset();
  serialResetPending = false;
}
