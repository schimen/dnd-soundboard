add_executable(test_keystate test_keystate.cpp)
target_include_directories(test_keystate PRIVATE ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(test_keystate PRIVATE Catch2::Catch2WithMain)
add_test(NAME KeystateTests COMMAND test_keystate)

add_executable(test_command test_command.cpp)
target_include_directories(test_command PRIVATE ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(test_command PRIVATE commands Catch2::Catch2WithMain)
add_test(NAME CommandTests COMMAND test_command)

add_executable(test_command_queue test_command_queue.cpp)
target_include_directories(test_command_queue PRIVATE ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(test_command_queue PRIVATE commands Catch2::Catch2WithMain)
add_test(NAME CommandQueueTests COMMAND test_command_queue)
