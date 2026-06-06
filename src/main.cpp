#include "pinouts/waveshare_rp2040_zero.h"
#include <pico/stdlib.h>
#include <Joystick.h>
#include <hardware/adc.h>
#include <utility>
#include <usb_descriptors.h>

using std::to_underlying;

enum class ControllerButtons : uint8_t {
  GEAR_1 = 0,
  GEAR_2,
  GEAR_3,
  GEAR_4,
  GEAR_5,
  GEAR_6,
  GEAR_R,
  RANGE,
  SPLIT,
  ENGINE_BRAKE,
  COUNT // Marchas (6 + R) e botões da manopla (3)
};

// Número de botões lógicos reportados pelo Joystick
const auto BUTTON_COUNT = to_underlying(ControllerButtons::COUNT); // Marchas (6 + R) e botões da manopla (3)
const bool initAutoSendState = true;


int analogRead(uint8_t pin);
void loop();
bool detectHandleConnection();

int main() {
  using enum ControllerButtons;

  bool handleConnected = false;
  static bool isReverseGear = false;

  stdio_init_all();

  Joystick.begin();
  Joystick.useManualSend(!initAutoSendState);

  gpio_init(SW_FRONT);
  gpio_set_dir(SW_FRONT, GPIO_IN);
  gpio_pull_up(SW_FRONT);
  gpio_init(SW_LEFT);
  gpio_set_dir(SW_LEFT, GPIO_IN);
  gpio_pull_up(SW_LEFT);
  gpio_init(SW_RIGHT);
  gpio_set_dir(SW_RIGHT, GPIO_IN);
  gpio_pull_up(SW_RIGHT);
  gpio_init(SW_BACK);
  gpio_set_dir(SW_BACK, GPIO_IN);
  gpio_pull_up(SW_BACK);
  gpio_init(SW_REVERSE);
  gpio_set_dir(SW_REVERSE, GPIO_IN);
  gpio_pull_up(SW_REVERSE);

  sleep_ms(2000);

  handleConnected = detectHandleConnection();

  bool swEnableReverse = (gpio_get(SW_ENABLE_REVERSE) == false);
  bool swEnableSequential = (gpio_get(SW_ENABLE_SEQUENTIAL) == false);

  if (!handleConnected) {
    Joystick.setButton(to_underlying(RANGE), false);
    Joystick.setButton(to_underlying(SPLIT), false);
    Joystick.setButton(to_underlying(ENGINE_BRAKE), false);

    printf("WARN: Truck shifter handle not connected\n");
  } else {
    gpio_init(SW_KNOB_RANGE);
    gpio_set_dir(SW_KNOB_RANGE, GPIO_IN);
    gpio_pull_up(SW_KNOB_RANGE);
    gpio_init(SW_KNOB_SPLIT);
    gpio_set_dir(SW_KNOB_SPLIT, GPIO_IN);
    gpio_pull_up(SW_KNOB_SPLIT);
    gpio_init(BTN_KNOB_ENGINE_BRAKE);
    gpio_set_dir(BTN_KNOB_ENGINE_BRAKE, GPIO_IN);
    gpio_pull_up(BTN_KNOB_ENGINE_BRAKE);

    printf("OK: Handle detected\n");
  }

  printf(swEnableReverse ? "OK: Rear gear is enabled\n" : "INFO: Rear gear is disabled\n");
  printf(swEnableSequential ? "OK: The current gear output is sequential\n" : "INFO: The current gear output is H-Shifter\n");

  while (true) {
    static bool prevButtonState[BUTTON_COUNT] = { false };

    bool swFront = (gpio_get(SW_FRONT) == false);
    bool swLeft = (gpio_get(SW_LEFT) == false);
    bool swRight = (gpio_get(SW_RIGHT) == false);
    bool swBack = (gpio_get(SW_BACK) == false);
    bool swReverse = (gpio_get(SW_REVERSE) == false);

    bool swRange = (gpio_get(SW_KNOB_RANGE) == false);
    bool swSplit = (gpio_get(SW_KNOB_SPLIT) == false);
    bool btnEngineBrake = (gpio_get(BTN_KNOB_ENGINE_BRAKE) == false);

    bool newButtonState[BUTTON_COUNT] = { false };

    bool combFrontUsed = false;
    bool combBackUsed = false;

    // Combinações para marchas laterais
    if (swFront && swLeft && !isReverseGear) { newButtonState[to_underlying(GEAR_1)] = true; combFrontUsed = true; }
    if (swLeft && swBack) { newButtonState[to_underlying(GEAR_2)] = true; combBackUsed = true; }
    if (swFront && swRight) { newButtonState[to_underlying(GEAR_5)] = true; combFrontUsed = true; }
    if (swRight && swBack) { newButtonState[to_underlying(GEAR_6)] = true; combBackUsed = true; }

    // Marchas centrais
    if (swFront && !combFrontUsed) newButtonState[to_underlying(GEAR_3)] = true;
    if (swBack && !combBackUsed) newButtonState[to_underlying(GEAR_4)] = true;

    //* Sei que a marcha ré não funciona assim, pois precisa que swReverse esteja sempre pressionado, talvez fazer algo mecânico para resolver isso.
    if (swReverse && combFrontUsed) newButtonState[to_underlying(GEAR_R)] = true;

    // Botões da manopla de caminhão
    if (handleConnected) {
      if (swRange) newButtonState[to_underlying(RANGE)] = true;
      if (swSplit) newButtonState[to_underlying(SPLIT)] = true;
      if (btnEngineBrake) newButtonState[to_underlying(ENGINE_BRAKE)] = true;
    }

    for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
      if (newButtonState[i] != prevButtonState[i]) {
        Joystick.setButton(i, newButtonState[i]);
        prevButtonState[i] = newButtonState[i];
      }
    }
    sleep_ms(20);
  }
}

bool detectHandleConnection() {
  adc_init();
  adc_gpio_init(SW_KNOB_RANGE);
  adc_select_input(SW_KNOB_RANGE - __FIRSTANALOGGPIO);

  int adc = adc_read() >> 2; // Leitura em 10 bits
  return (adc > 300 && adc < 600);
}
