message(STATUS "hal_transport_usb available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_transport_usb INTERFACE)

target_sources(hal_transport_usb INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/transport_usb.c
)

target_link_libraries(hal_transport_usb INTERFACE
    hal_usb
)

target_include_directories(hal_transport_usb INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)