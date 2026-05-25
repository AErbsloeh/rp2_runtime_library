message(STATUS "hal_clk available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_clk INTERFACE)

target_sources(hal_clk INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/pio_clock.c
)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/pio_clock.pio.h")
    message(STATUS "Pio Header for pio_clock.pio.h already exists, skipping generation.")
else()
    pico_generate_pio_header(hal_clk ${CMAKE_CURRENT_LIST_DIR}/pio_clock.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()


target_link_libraries(hal_clk INTERFACE
    pico_stdlib
    hardware_clocks
    hardware_pio
)

target_include_directories(hal_clk INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
