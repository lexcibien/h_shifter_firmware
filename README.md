# H Shifter Firmware - RP2040 Zero

Firmware for a H Shifter with RP2040 Zero embedded, read 8 inputs and shows 12 virtual buttons to the PC as a Joystick.
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
