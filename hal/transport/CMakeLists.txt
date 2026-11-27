message(STATUS "hal_transport available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_transport INTERFACE)

target_include_directories(hal_transport INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

message(STATUS "hal_transport_usb available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_transport_usb INTERFACE)

target_sources(hal_transport_usb INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/transport_usb.c
)

target_link_libraries(hal_transport_usb INTERFACE
    hal_transport
    hal_usb
)

target_include_directories(hal_transport_usb INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

message(STATUS "hal_transport_wifi available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_transport_wifi INTERFACE)

target_sources(hal_transport_wifi INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/transport_wifi.c
)

target_link_libraries(hal_transport_wifi INTERFACE
    pico_stdio_usb
    hal_transport
    hal_wifi_tcp
)

target_include_directories(hal_transport_wifi INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

message(STATUS "hal_transport_bt available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_transport_bt INTERFACE)

target_sources(hal_transport_bt INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/transport_bt.c
)

target_link_libraries(hal_transport_bt INTERFACE
    pico_stdio_usb
    hal_transport
    hal_btspp
)

target_include_directories(hal_transport_bt INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
