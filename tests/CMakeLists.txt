add_test(NAME memory_size_tests COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR})

find_program(NODE_EXECUTABLE node)
if(NODE_EXECUTABLE)
  add_test(NAME web_protocol_tests COMMAND ${NODE_EXECUTABLE} --test ${PROJECT_SOURCE_DIR}/web/protocol.test.js)
endif()
