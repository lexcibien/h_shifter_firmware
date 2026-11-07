#include <ShifterInput.h>
#include <hardware/adc.h>
#include <pico/time.h>

bool ShifterInput::readDebounced(Button& button) {
  const bool raw = button.activeLow ? !gpio_get(button.pin) : gpio_get(button.pin);

  if (raw != button.lastRaw) {
    button.lastChange = to_ms_since_boot(get_absolute_time());
    button.lastRaw = raw;
  }
  if ((to_ms_since_boot(get_absolute_time()) - button.lastChange) > DEBOUNCE_MS) {
    button.stableState = raw;
  }

  return button.stableState;
}

void ShifterInput::begin() {
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
  handleConnected = detectHandleConnection();

  if (handleConnected) {
    configureHandle();
  }
}

bool ShifterInput::detectHandleConnection() {
  adc_select_input(SW_KNOB_RANGE - ADC_BASE_PIN);
  const int DETECT_VALUE = TARGET_ANG_VALUE + 700; // Needs to be less than 4095
  const int adc = adc_read();
  return adc < DETECT_VALUE;
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

  handleConfigured = true;
}

void ShifterInput::checkSequential() {
#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  if (const bool isPressed = readDebounced(sequentialModeButton); !isPressed) {
    sequentialPressActive = false;
    sequentialPressHandled = false;
    sequentialPressStartTime = 0;
    return;
  }

  if (!sequentialPressActive) {
    sequentialPressActive = true;
    sequentialPressStartTime = to_ms_since_boot(get_absolute_time());
    return;
  }

  if (!sequentialPressHandled && (to_ms_since_boot(get_absolute_time()) - sequentialPressStartTime) >= SEQUENTIAL_MODE_HOLD_TIME_MS) {
    sequentialEnabled = !sequentialEnabled;
    sequentialPressHandled = true;
  }
#endif
}

ShifterInput::AnalogState ShifterInput::readAnalogInput() {
  ShifterInput::AnalogState input;

  adc_select_input(SW_KNOB_RANGE - ADC_BASE_PIN);
  input.adc = adc_read();
  input.voltage = static_cast<float>(input.adc) * CONVERSION_FACTOR;

  return input;
}

ShifterModel::InputState ShifterInput::readInputs() {
  ShifterModel::InputState inputs;

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  inputs.swFront = readDebounced(frontButton);
  inputs.swLeft = readDebounced(leftButton);
  inputs.swRight = readDebounced(rightButton);
  inputs.swBack = readDebounced(backButton);
  inputs.swReverse = readDebounced(reverseButton);

  checkSequential();
#elif SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V2
  inputs.hPatternMode = readDebounced(modeButton);
  inputs.sw3 = readDebounced(button3);
  inputs.sw4 = readDebounced(button4);
  inputs.sw5 = readDebounced(button5);
  inputs.sw6 = readDebounced(button6);
  sequentialEnabled = !inputs.hPatternMode;
#endif

  if (to_ms_since_boot(get_absolute_time()) - lastScan >= 500) {
    handleConnected = detectHandleConnection();
    if (handleConnected && !handleConfigured) {
      configureHandle();
    }
    lastScan = to_ms_since_boot(get_absolute_time());
  }

  if (handleConnected) {
    inputs.swRange = readDebounced(rangeButton);
    inputs.swSplit = readDebounced(splitButton);
    inputs.btnEngineBrake = readDebounced(engineBrakeButton);
  }

  return inputs;
}

ShifterModel::ShifterConfig ShifterInput::configuration() { return { .sequentialEnabled = sequentialEnabled, .handleConnected = handleConnected }; }
