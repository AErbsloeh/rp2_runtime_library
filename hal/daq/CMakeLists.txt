message(STATUS "hal_daq available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_daq INTERFACE)

target_sources(hal_daq INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/fifo.c
)

target_link_libraries(hal_daq INTERFACE
	pico_stdlib
    hardware_dma
)

target_include_directories(hal_daq INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)