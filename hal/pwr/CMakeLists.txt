message(STATUS "hal_pwr_single available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_pwr_single INTERFACE)

target_sources(hal_pwr_single INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/pwr_single.c
)

target_link_libraries(hal_pwr_single INTERFACE
	pico_stdlib
	hardware_gpio
)

target_include_directories(hal_pwr_single INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

message(STATUS "hal_pwr_dual available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_pwr_dual INTERFACE)

target_sources(hal_pwr_dual INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/pwr_dual.c
)

target_link_libraries(hal_pwr_dual INTERFACE
	pico_stdlib
	hardware_gpio
)

target_include_directories(hal_pwr_dual INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
