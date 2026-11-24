#include "ShifterInput.h"
#include <algorithm>
#include <array>
#include <hardware/adc.h>
#include <pico/time.h>

bool ShifterInput::readDebounced(Button& button) {
  const bool raw = button.activeLow ? !gpio_get(button.pin) : gpio_get(button.pin);

  if (raw != button.lastRaw) {
    button.lastChange = nowMs;
    button.lastRaw = raw;
  }
  if (nowMs - button.lastChange > deviceConfig.debounceMs) {
    button.stableState = raw;
  }

  return button.stableState;
}

void ShifterInput::resetButton(Button& button) {
  button.lastRaw = false;
  button.stableState = false;
  button.lastChange = nowMs;
}

void ShifterInput::begin(const DeviceConfig& config) {
  deviceConfig = config;
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  sequentialEnabled = config.defaultSequentialMode;
#endif
  adc_init();
  adc_gpio_init(SW_KNOB_RANGE);

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
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
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  gpio_init(PIN_3);
  gpio_set_dir(PIN_3, GPIO_IN);
  gpio_pull_up(PIN_3);

  gpio_init(PIN_4);
  gpio_set_dir(PIN_4, GPIO_IN);
  gpio_pull_up(PIN_4);

  gpio_init(PIN_5);
  gpio_set_dir(PIN_5, GPIO_IN);
  gpio_pull_up(PIN_5);

  gpio_init(PIN_6);
  gpio_set_dir(PIN_6, GPIO_IN);
  gpio_pull_up(PIN_6);

  gpio_init(PIN_MODE);
  gpio_set_dir(PIN_MODE, GPIO_IN);
  gpio_pull_up(PIN_MODE);
#endif

  sleep_ms(2000);
  redetectHandle();
}

bool ShifterInput::detectHandleConnection() {
  adc_select_input(SW_KNOB_RANGE - ADC_BASE_PIN);
  std::array<int, 9> samples{};
  for (int& sample : samples) {
    sample = adc_read();
    sleep_us(100);
  }
  std::sort(samples.begin(), samples.end());
  lastAdc = samples[samples.size() / 2];
  lastVoltage = static_cast<float>(lastAdc) * CONVERSION_FACTOR;
  return lastAdc < deviceConfig.handleDetectThreshold;
}

void ShifterInput::configureHandle() {
  gpio_set_function(SW_KNOB_RANGE, GPIO_FUNC_SIO);
  gpio_set_dir(SW_KNOB_RANGE, GPIO_IN);

  gpio_init(SW_KNOB_SPLIT);
  gpio_set_dir(SW_KNOB_SPLIT, GPIO_IN);
  gpio_pull_up(SW_KNOB_SPLIT);

  gpio_init(BTN_KNOB_ENGINE_BRAKE);
  gpio_set_dir(BTN_KNOB_ENGINE_BRAKE, GPIO_IN);
  gpio_pull_up(BTN_KNOB_ENGINE_BRAKE);
}

bool ShifterInput::redetectHandle() {
  adc_gpio_init(SW_KNOB_RANGE);
  sleep_ms(10);
  handleConnected = detectHandleConnection();
  if (handleConnected) {
    configureHandle();
  }
  return handleConnected;
}

void ShifterInput::setConfig(const DeviceConfig& config) {
  deviceConfig = config;
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  resetButton(frontButton);
  resetButton(leftButton);
  resetButton(rightButton);
  resetButton(backButton);
  resetButton(reverseButton);
  resetButton(sequentialModeButton);
#else
  resetButton(modeButton);
  resetButton(button3);
  resetButton(button4);
  resetButton(button5);
  resetButton(button6);
#endif
  resetButton(rangeButton);
  resetButton(splitButton);
  resetButton(engineBrakeButton);
}

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
void ShifterInput::checkSequential(bool isPressed, bool suppressToggle) {
  if (!isPressed || suppressToggle) {
    sequentialPressActive = false;
    sequentialPressHandled = false;
    sequentialPressStartTime = 0;
    return;
  }

  if (!sequentialPressActive) {
    sequentialPressActive = true;
    sequentialPressStartTime = nowMs;
    return;
  }

  if (!sequentialPressHandled && (nowMs - sequentialPressStartTime) >= deviceConfig.sequentialHoldMs) {
    sequentialEnabled = !sequentialEnabled;
    sequentialPressHandled = true;
  }
}
#endif

ShifterInput::AnalogState ShifterInput::readAnalogInput() { return { .adc = lastAdc, .voltage = lastVoltage }; }

ShifterModel::InputState ShifterInput::readInputs() {
  ShifterModel::InputState inputs;
  nowMs = to_ms_since_boot(get_absolute_time());

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  inputs.swFront = readDebounced(frontButton);
  inputs.swLeft = readDebounced(leftButton);
  inputs.swRight = readDebounced(rightButton);
  inputs.swBack = readDebounced(backButton);
  inputs.swReverse = readDebounced(reverseButton);
  const bool sequentialButtonPressed = readDebounced(sequentialModeButton);
  unlockGesture = sequentialButtonPressed && inputs.swRight;
  checkSequential(sequentialButtonPressed, unlockGesture);
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  inputs.hPatternMode = readDebounced(modeButton);
  inputs.sw3 = readDebounced(button3);
  inputs.sw4 = readDebounced(button4);
  inputs.sw5 = readDebounced(button5);
  inputs.sw6 = readDebounced(button6);
  sequentialEnabled = !inputs.hPatternMode;
  unlockGesture = sequentialEnabled && inputs.sw5;
#endif

  if (handleConnected) {
    inputs.swRange = readDebounced(rangeButton);
    inputs.swSplit = readDebounced(splitButton);
    inputs.btnEngineBrake = readDebounced(engineBrakeButton);
  }

  return inputs;
}

ShifterModel::ShifterConfig ShifterInput::configuration() {
  return {
    .sequentialEnabled = sequentialEnabled,
    .handleConnected = handleConnected,
    .reverseEnabled = deviceConfig.reverseEnabled,
    .forwardGearCount = deviceConfig.forwardGearCount,
  };
}

bool ShifterInput::configurationUnlockGesture() { return unlockGesture; }
