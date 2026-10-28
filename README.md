# H Shifter Firmware - RP2040 Zero

Firmware for an H Shifter with an embedded RP2040 Zero. It exposes 12 virtual buttons in project V1 and 13 in project V2.
It was created for the H Shifter from DAZ Projects.

I developed a switch holder and a lifter for a external rear gear, similar to car with a physical lifter lock out mechanism to let it engage, only it's
electronically simulated.

![lifter image](url)

## Dependencies

- Platformio

Some branches use CMake for compiling:

- CMake
- Clangd
- clang-tidy
- ClangFormat
- Ninja

## Compiling

You can compile this project using platformio by now, but there are branches that uses CMake to do this job for you.

### Selecting the project version

The firmware contains both hardware mappings and gear logics:

- `SHIFTER_PROJECT_V1`: 6 gears + reverse and optional sequential mode.
- `SHIFTER_PROJECT_V2`: five-switch design with 7 gears + reverse and a physical H/sequential selector.

Both versions support the truck shifter handle (range, split and engine brake). Select the project through the PlatformIO environment:

```bash
pio run -e waveshare_rp2040_zero_v1
pio run -e waveshare_rp2040_zero_v2
```

Running `pio run` without `-e` builds V2, which is configured as `default_envs` in `platformio.ini`.

There are two ways to compile with CMake:

- By using the extension CMakeTools on VSCode
- Using the command:

```bash
cmake --build build
```

## Flashing

Programming on platformio is as straight forward as compiling the firmware. On CMake it uses a target command to flash in the microcontroller:

```bash
cmake --build build --target flash
```

## License

- [**MIT License**](https://github.com/lexcibien/h_shifter_firmware/blob/main/LICENSE)
