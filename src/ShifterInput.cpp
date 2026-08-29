#include "hardware/adc.h"
#include <ShifterInput.h>

void ShifterInput::begin() {
  adc_init();
  adc_gpio_init(SW_KNOB_RANGE);

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
  gpio_init(SW_ENABLE_SEQUENTIAL);
  gpio_set_dir(SW_ENABLE_SEQUENTIAL, GPIO_IN);
  gpio_pull_up(SW_ENABLE_SEQUENTIAL);

  sleep_ms(2000);
  handleConnected = detectHandleConnection();

  if (handleConnected) {
    gpio_set_function(SW_KNOB_RANGE, GPIO_FUNC_SIO);
    gpio_set_dir(SW_KNOB_RANGE, GPIO_IN);

    gpio_init(SW_KNOB_SPLIT);
    gpio_set_dir(SW_KNOB_SPLIT, GPIO_IN);
    gpio_pull_up(SW_KNOB_SPLIT);

    gpio_init(BTN_KNOB_ENGINE_BRAKE);
    gpio_set_dir(BTN_KNOB_ENGINE_BRAKE, GPIO_IN);
    gpio_pull_up(BTN_KNOB_ENGINE_BRAKE);
  }
}

ShifterInput::AnalogState ShifterInput::readAnalogInput() {
  ShifterInput::AnalogState input;

  adc_select_input(SW_KNOB_RANGE - ADC_BASE_PIN);
  input.adc = adc_read();
  input.voltage = static_cast<float>(input.adc) * CONVERSION_FACTOR;

  return input;
}

[[nodiscard]] ShifterModel::InputState ShifterInput::readInputs() const {
  ShifterModel::InputState inputs;
  inputs.swFront = !gpio_get(SW_FRONT);
  inputs.swLeft = !gpio_get(SW_LEFT);
  inputs.swRight = !gpio_get(SW_RIGHT);
  inputs.swBack = !gpio_get(SW_BACK);
  inputs.swReverse = !gpio_get(SW_REVERSE);

  if (handleConnected) {
    inputs.swRange = !gpio_get(SW_KNOB_RANGE);
    inputs.swSplit = !gpio_get(SW_KNOB_SPLIT);
    inputs.btnEngineBrake = !gpio_get(BTN_KNOB_ENGINE_BRAKE);
  }

  return inputs;
}

[[nodiscard]] ShifterModel::ShifterConfig ShifterInput::configuration() const {
  return { .sequentialEnabled = !gpio_get(SW_ENABLE_SEQUENTIAL), .handleConnected = handleConnected };
}
