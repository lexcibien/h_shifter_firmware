include_guard(GLOBAL)

function(h_shifter_add_post_build_steps target)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${PROJECT_SOURCE_DIR}/compile_commands.json
    VERBATIM
  )

  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "=== Memory Usage Report ==="
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/memory_size.py
      --file $<TARGET_FILE:${target}>
      --board ${PICO_BOARD}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    VERBATIM
  )
endfunction()

function(h_shifter_add_upload_target target)
  add_custom_target(upload
    COMMAND picotool load -f -x $<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.bin -o 0x10000000
    DEPENDS ${target}
    VERBATIM
  )
endfunction()
