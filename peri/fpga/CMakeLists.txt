message(STATUS "peri_fpga_flash available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(peri_fpga_flash INTERFACE)

# --- Fetch files ---
target_sources(peri_fpga_flash INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/flash.c
)

target_link_libraries(peri_fpga_flash INTERFACE
    pico_stdlib
	hal_spi
)

# --- Link to project ---
target_include_directories(peri_fpga_flash INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)


message(STATUS "peri_fpga_logic available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(peri_fpga_logic INTERFACE)

# --- Fetch files ---
target_sources(peri_fpga_logic INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/logic.c
)

target_link_libraries(peri_fpga_logic INTERFACE
    pico_stdlib
	hal_spi
)

# --- Link to project ---
target_include_directories(peri_fpga_logic INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
