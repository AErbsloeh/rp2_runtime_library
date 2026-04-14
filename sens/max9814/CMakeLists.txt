message(STATUS "sens_max9814 available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(sens_max9814 INTERFACE)

target_sources(sens_max9814 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/max9814.c
)

target_link_libraries(sens_max9814 INTERFACE
	pico_stdlib
	hardware_gpio
)

target_include_directories(sens_max9814 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
