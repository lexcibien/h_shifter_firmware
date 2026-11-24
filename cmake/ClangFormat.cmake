include_guard(GLOBAL)

function(h_shifter_collect_c_cpp_sources output_variable)
  file(GLOB_RECURSE sources CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${PROJECT_SOURCE_DIR}/src/*.c"
    "${PROJECT_SOURCE_DIR}/src/*.cc"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.cxx"
    "${PROJECT_SOURCE_DIR}/src/*.h"
    "${PROJECT_SOURCE_DIR}/src/*.hh"
    "${PROJECT_SOURCE_DIR}/src/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/*.hxx"
  )

  list(SORT sources)
  set(${output_variable} "${sources}" PARENT_SCOPE)
endfunction()

function(h_shifter_add_clang_format_targets)
  h_shifter_collect_c_cpp_sources(sources)
  find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)

  if(NOT sources)
    message(WARNING "Nenhum arquivo C/C++ foi encontrado para o clang-format")
    add_custom_target(format
      COMMAND ${CMAKE_COMMAND} -E echo "Nenhum arquivo C/C++ foi encontrado para o clang-format"
      COMMAND ${CMAKE_COMMAND} -E false
      VERBATIM
    )
    add_custom_target(format-check
      COMMAND ${CMAKE_COMMAND} -E echo "Nenhum arquivo C/C++ foi encontrado para o clang-format"
      COMMAND ${CMAKE_COMMAND} -E false
      VERBATIM
    )
    return()
  endif()

  if(NOT CLANG_FORMAT_EXECUTABLE)
    message(WARNING "clang-format não encontrado; os targets format e format-check irão falhar até que ele seja instalado")
    add_custom_target(format
      COMMAND ${CMAKE_COMMAND} -E echo "clang-format não encontrado no PATH"
      COMMAND ${CMAKE_COMMAND} -E false
      VERBATIM
    )
    add_custom_target(format-check
      COMMAND ${CMAKE_COMMAND} -E echo "clang-format não encontrado no PATH"
      COMMAND ${CMAKE_COMMAND} -E false
      VERBATIM
    )
    return()
  endif()

  add_custom_target(format
    COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${sources}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Formatando fontes C/C++ com clang-format"
    VERBATIM
  )

  add_custom_target(format-check
    COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run --Werror ${sources}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Verificando formatação C/C++ com clang-format"
    VERBATIM
  )
endfunction()
