# Key monitor
add_executable(key_monitor key_monitor.cpp)
target_include_directories(key_monitor PRIVATE ${CMAKE_SOURCE_DIR}/libs ${LIBEVDEV_INCLUDE_DIRS})
target_link_libraries(key_monitor PRIVATE event_device commands argparse::argparse spdlog::spdlog ${LIBEVDEV_LIBRARIES})
install(TARGETS key_monitor DESTINATION bin)

# Sound player
add_executable(sound_player sound_player.cpp)
target_link_libraries(sound_player PRIVATE SDL2::SDL2 SDBusCpp::sdbus-c++)

install(TARGETS key_monitor sound_player DESTINATION BINDIR)
