add_library(event_device)
target_include_directories(event_device PRIVATE ${LIBEVDEV_INCLUDE_DIRS})
target_link_libraries(event_device PRIVATE ${LIBEVDEV_LIBRARIES})
target_sources(event_device PRIVATE event_device.cpp event_device.h keystate.h)
