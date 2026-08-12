message(STATUS "peri_w5500 available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(peri_w5500 INTERFACE)

# --- Fetch external libs from GitHub ---
target_sources(peri_w5500 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/w5500_udp.c
)
target_link_libraries(peri_w5500 INTERFACE
    pico_stdlib
    hardware_gpio
    hal_spi
    hardware_dma
)

# --- Link to project ---
target_include_directories(peri_w5500 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
