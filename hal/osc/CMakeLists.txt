message(STATUS "hal_osc available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_osc INTERFACE)

target_sources(hal_osc INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/osc.c
)

target_link_libraries(hal_osc INTERFACE
	pico_stdlib
	hardware_clocks
)

target_include_directories(hal_osc INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
