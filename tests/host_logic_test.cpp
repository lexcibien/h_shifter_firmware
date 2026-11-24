#include "config/DeviceConfig.h"
#include "shifter/ShifterLogic.h"
#include <cassert>

int main() {
  DeviceConfig config = DeviceConfig::defaults();
  assert(config.isValid());
  const auto encoded = config.encode();
  DeviceConfig decoded;
  assert(DeviceConfig::decode(encoded, decoded));
  assert(decoded == config);

  ShifterModel::ShifterConfig runtime{
    .sequentialEnabled = false,
    .handleConnected = false,
    .reverseEnabled = false,
    .forwardGearCount = 4,
  };
  ShifterModel::InputState input{};

#if SHIFTER_PROJECT_VERSION == SHIFTER_PROJECT_V1
  input.swFront = true;
  input.swLeft = true;
  auto buttons = ShifterLogic::resolveButtonState(input, runtime);
  assert(!buttons[ShifterModel::GEAR_6]);

  input = {};
  input.swReverse = true;
  input.swBack = true;
  input.swRight = true;
  buttons = ShifterLogic::resolveButtonState(input, runtime);
  assert(!buttons[ShifterModel::GEAR_R]);
  assert(!buttons[ShifterModel::GEAR_1]);
#else
  input.sw3 = true;
  input.sw4 = true;
  input.sw5 = true;
  auto buttons = ShifterLogic::resolveButtonState(input, runtime);
  assert(!buttons[ShifterModel::GEAR_7]);
  assert(!buttons[ShifterModel::GEAR_1]);

  input = {};
  input.sw6 = true;
  input.sw4 = true;
  input.sw5 = true;
  buttons = ShifterLogic::resolveButtonState(input, runtime);
  assert(!buttons[ShifterModel::GEAR_R]);
  assert(!buttons[ShifterModel::GEAR_2]);
#endif
}
