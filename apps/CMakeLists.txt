# Key monitor
add_executable(key_monitor key_monitor.cpp)
target_include_directories(key_monitor PRIVATE ${CMAKE_SOURCE_DIR}/libs ${LIBEVDEV_INCLUDE_DIRS})
target_link_libraries(key_monitor PRIVATE event_device commands argparse::argparse ${LIBEVDEV_LIBRARIES})
install(TARGETS key_monitor DESTINATION bin)

# Sound player
add_executable(sound_player sound_player.cpp)
target_include_directories(sound_player PRIVATE ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(sound_player PRIVATE commands sample argparse::argparse SDL3::SDL3 SDL3_mixer::SDL3_mixer SDBusCpp::sdbus-c++)
install(TARGETS sound_player DESTINATION bin)
