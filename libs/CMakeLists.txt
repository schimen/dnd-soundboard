add_library(event_device)
target_include_directories(event_device PRIVATE ${LIBEVDEV_INCLUDE_DIRS})
target_link_libraries(event_device PRIVATE ${LIBEVDEV_LIBRARIES})
target_sources(event_device PRIVATE event_device.cpp event_device.h keystate.hpp)

add_library(commands)
target_sources(commands PRIVATE commands.cpp commands.h)

add_library(sample)
target_sources(sample PRIVATE sample.cpp sample.h)
