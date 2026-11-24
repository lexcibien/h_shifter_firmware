include_guard(GLOBAL)

function(h_shifter_get_compiler_system_include_args output_variable)
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -E -x c++ - -v
    INPUT_FILE /dev/null
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    RESULT_VARIABLE compiler_result
    ERROR_VARIABLE compiler_stderr
    OUTPUT_QUIET
  )

  if(NOT compiler_result EQUAL 0)
    message(WARNING "Não foi possível consultar os headers da toolchain para o clang-tidy")
    set(${output_variable} "" PARENT_SCOPE)
    return()
  endif()

  string(REPLACE "\r\n" "\n" compiler_stderr "${compiler_stderr}")
  string(REPLACE "\n" ";" compiler_lines "${compiler_stderr}")
  set(includes_started false)
  set(include_directories)
  foreach(line IN LISTS compiler_lines)
    string(STRIP "${line}" line)
    if(line STREQUAL "#include <...> search starts here:")
      set(includes_started true)
    elseif(includes_started AND line STREQUAL "End of search list.")
      set(includes_started false)
    elseif(includes_started AND line)
      list(APPEND include_directories "${line}")
    endif()
  endforeach()

  if(NOT include_directories)
    message(WARNING "A lista de headers da toolchain está vazia para o clang-tidy")
    set(${output_variable} "" PARENT_SCOPE)
    return()
  endif()

  set(include_args)
  foreach(include_directory IN LISTS include_directories)
    string(STRIP "${include_directory}" include_directory)
    if(include_directory)
      list(APPEND include_args "--extra-arg=-isystem${include_directory}")
    endif()
  endforeach()
  set(${output_variable} "${include_args}" PARENT_SCOPE)
endfunction()

function(h_shifter_configure_clang_tidy target)
  if(NOT H_SHIFTER_ENABLE_CLANG_TIDY)
    return()
  endif()

  find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
  h_shifter_get_compiler_system_include_args(system_include_args)

  set(compiler_target "${CMAKE_CXX_COMPILER_TARGET}")
  if(NOT compiler_target AND PICO_GCC_TRIPLE)
    set(compiler_target "${PICO_GCC_TRIPLE}")
  endif()
  if(NOT compiler_target)
    set(compiler_target "arm-none-eabi")
  endif()

  set(clang_tidy_command
    "${CLANG_TIDY_EXECUTABLE}"
    "--quiet"
    "--config-file=${PROJECT_SOURCE_DIR}/.clang-tidy"
    "--extra-arg=-Wno-unknown-warning-option"
    "--extra-arg-before=--target=${compiler_target}"
    ${system_include_args}
  )
  set_property(TARGET ${target} PROPERTY C_CLANG_TIDY "${clang_tidy_command}")
  set_property(TARGET ${target} PROPERTY CXX_CLANG_TIDY "${clang_tidy_command}")
endfunction()

function(h_shifter_add_clang_tidy_target target)
  find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)

  if(NOT CLANG_TIDY_EXECUTABLE)
    message(WARNING "clang-tidy não encontrado; o target lint irá falhar até que ele seja instalado")
    add_custom_target(lint
      COMMAND ${CMAKE_COMMAND} -E echo "clang-tidy não encontrado no PATH"
      COMMAND ${CMAKE_COMMAND} -E false
      VERBATIM
    )
    return()
  endif()

  h_shifter_configure_clang_tidy(${target})

  get_target_property(target_sources ${target} SOURCES)
  get_target_property(target_source_directory ${target} SOURCE_DIR)
  set(sources)

  foreach(source IN LISTS target_sources)
    if(NOT source MATCHES "\\.(c|cc|cpp|cxx)$")
      continue()
    endif()

    if(IS_ABSOLUTE "${source}")
      set(absolute_source "${source}")
    else()
      get_filename_component(absolute_source
        "${target_source_directory}/${source}"
        ABSOLUTE
      )
    endif()
    list(APPEND sources "${absolute_source}")
  endforeach()

  if(NOT sources)
    message(FATAL_ERROR "Nenhum fonte C/C++ compilável foi encontrado no target ${target} para o clang-tidy")
  endif()

  if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
    message(FATAL_ERROR "CMAKE_EXPORT_COMPILE_COMMANDS precisa estar habilitado para o target lint")
  endif()

  h_shifter_get_compiler_system_include_args(system_include_args)
  set(compiler_target "${CMAKE_CXX_COMPILER_TARGET}")
  if(NOT compiler_target AND PICO_GCC_TRIPLE)
    set(compiler_target "${PICO_GCC_TRIPLE}")
  endif()
  if(NOT compiler_target)
    set(compiler_target "arm-none-eabi")
  endif()

  add_custom_target(lint
    COMMAND ${CLANG_TIDY_EXECUTABLE}
      --quiet
      --config-file=${PROJECT_SOURCE_DIR}/.clang-tidy
      -p=${CMAKE_BINARY_DIR}
      ${sources}
      --extra-arg=-Wno-unknown-warning-option
      --extra-arg-before=--target=${compiler_target}
      ${system_include_args}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Analisando fontes C/C++ com clang-tidy"
    VERBATIM
  )
  add_dependencies(lint ${target})
endfunction()
