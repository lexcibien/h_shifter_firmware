cmake_minimum_required(VERSION 3.24)
project(DazShifterDesktop LANGUAGES NONE)

find_program(DAZ_CARGO NAMES cargo REQUIRED)
find_program(DAZ_RUSTC NAMES rustc REQUIRED)
execute_process(
  COMMAND "${DAZ_RUSTC}" -vV
  OUTPUT_VARIABLE rustc_version
  COMMAND_ERROR_IS_FATAL ANY
)
string(REGEX MATCH "host: ([^ \r\n]+)" rust_host "${rustc_version}")
set(rust_target "${CMAKE_MATCH_1}")

set(manifest "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${manifest}")
file(STRINGS "${manifest}" version_line REGEX "^version[ \t]*=" LIMIT_COUNT 1)
if(NOT version_line MATCHES "\"([0-9]+\\.[0-9]+\\.[0-9]+)\"")
  message(FATAL_ERROR "Expected a numeric MAJOR.MINOR.PATCH version in ${manifest}")
endif()
set(DAZ_VERSION "${CMAKE_MATCH_1}")
set(DAZ_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
set(packaging "${CMAKE_CURRENT_SOURCE_DIR}/packaging")
set(dist "${CMAKE_CURRENT_BINARY_DIR}/dist")
set(cargo_target_dir "${CMAKE_CURRENT_SOURCE_DIR}/target")
set(cargo_env "CARGO_PROFILE_RELEASE_STRIP=symbols" "CARGO_TARGET_DIR=${cargo_target_dir}")

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND rust_target MATCHES "^(x86_64|aarch64)-unknown-linux-gnu$")
  set(platform linux)
  set(DAZ_ARCH "${CMAKE_MATCH_1}")
  set(extension ".sh")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows" AND rust_target STREQUAL "x86_64-pc-windows-msvc")
  set(platform windows)
  set(DAZ_ARCH x86_64)
  set(extension ".exe")
  set(exe_suffix ".exe")
  list(APPEND cargo_env "CARGO_TARGET_X86_64_PC_WINDOWS_MSVC_RUSTFLAGS=-C target-feature=+crt-static")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND rust_target MATCHES "^(x86_64|aarch64)-apple-darwin$")
  set(platform macos)
  set(DAZ_ARCH "${CMAKE_MATCH_1}")
  set(extension ".pkg")
  set(DAZ_MACOS_MIN_VERSION "15.0")
  set(DAZ_MACOS_PKG_ARCH "${DAZ_ARCH}")
  if(DAZ_ARCH STREQUAL "aarch64")
    set(DAZ_MACOS_PKG_ARCH arm64)
  endif()
  list(APPEND cargo_env "MACOSX_DEPLOYMENT_TARGET=${DAZ_MACOS_MIN_VERSION}")
else()
  message(FATAL_ERROR "Unsupported installer host ${CMAKE_HOST_SYSTEM_NAME} / ${rust_target}. Build on the target OS with its native Rust toolchain.")
endif()

set(DAZ_EXE "${cargo_target_dir}/${rust_target}/release/daz-shifter-desktop${exe_suffix}")
set(DAZ_ARTIFACT_NAME "daz-shifter-${DAZ_VERSION}-${platform}-${DAZ_ARCH}")
set(artifact "${dist}/${DAZ_ARTIFACT_NAME}${extension}")

add_custom_target(desktop-release
  COMMAND "${CMAKE_COMMAND}" -E env ${cargo_env}
    "${DAZ_CARGO}" build --locked --release --target "${rust_target}" --bin daz-shifter-desktop
  BYPRODUCTS "${DAZ_EXE}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  USES_TERMINAL
  VERBATIM
)

if(platform STREQUAL "linux")
  find_program(DAZ_BASH NAMES bash REQUIRED)
  configure_file("${packaging}/linux/install.sh.in" "${CMAKE_CURRENT_BINARY_DIR}/install-linux.sh" @ONLY)
  set(package_command
    "${DAZ_BASH}" "${packaging}/linux/build-installer.sh"
    "${CMAKE_CURRENT_BINARY_DIR}/install-linux.sh" "${DAZ_EXE}" "${DAZ_LICENSE}"
    "${packaging}/linux/70-daz-shifter.rules" "${artifact}"
  )
elseif(platform STREQUAL "windows")
  find_program(DAZ_ISCC NAMES ISCC.exe ISCC HINTS
    "$ENV{ProgramFiles\(x86\)}/Inno Setup 6"
    "$ENV{ProgramFiles\(x86\)}/Inno Setup 7"
    "$ENV{ProgramFiles}/Inno Setup 6"
    "$ENV{ProgramFiles}/Inno Setup 7"
    "$ENV{LOCALAPPDATA}/Programs/Inno Setup 6"
    "$ENV{LOCALAPPDATA}/Programs/Inno Setup 7"
    REQUIRED
  )
  cmake_path(NATIVE_PATH DAZ_EXE NORMALIZE DAZ_WINDOWS_EXE)
  cmake_path(NATIVE_PATH DAZ_LICENSE NORMALIZE DAZ_WINDOWS_LICENSE)
  cmake_path(NATIVE_PATH dist NORMALIZE DAZ_WINDOWS_DIST)
  configure_file("${packaging}/windows/setup.iss.in" "${CMAKE_CURRENT_BINARY_DIR}/setup.iss" @ONLY)
  set(package_command "${DAZ_ISCC}" "${CMAKE_CURRENT_BINARY_DIR}/setup.iss")
else()
  set(DAZ_MACOS_APP_SIGN_IDENTITY "-" CACHE STRING "codesign identity for the macOS app")
  set(DAZ_MACOS_INSTALLER_SIGN_IDENTITY "" CACHE STRING "Optional Developer ID Installer identity")
  configure_file("${packaging}/macos/Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" @ONLY)
  configure_file("${packaging}/macos/Distribution.xml.in" "${CMAKE_CURRENT_BINARY_DIR}/Distribution.xml" @ONLY)
  set(package_command
    "${CMAKE_COMMAND}"
    "-Dstage=${CMAKE_CURRENT_BINARY_DIR}/macos-stage"
    "-Dbundle_info=${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
    "-Ddistribution=${CMAKE_CURRENT_BINARY_DIR}/Distribution.xml"
    "-Dexecutable=${DAZ_EXE}" "-Dlicense=${DAZ_LICENSE}" "-Dversion=${DAZ_VERSION}"
    "-Dartifact=${artifact}" "-Dapp_sign_identity=${DAZ_MACOS_APP_SIGN_IDENTITY}"
    "-Dinstaller_sign_identity=${DAZ_MACOS_INSTALLER_SIGN_IDENTITY}"
    -P "${packaging}/macos/package.cmake"
  )
endif()

add_custom_target(installer-${platform}
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${dist}"
  COMMAND ${package_command}
  BYPRODUCTS "${artifact}"
  DEPENDS desktop-release
  USES_TERMINAL
  VERBATIM
)
add_custom_target(installer DEPENDS installer-${platform})
