message(STATUS "peri_oled4400 available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(peri_oled4400 INTERFACE)

# --- Fetch files ---
target_sources(peri_oled4400 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ssd1306.c
)

target_link_libraries(peri_oled4400 INTERFACE
    pico_stdlib
	hal_i2c
)

# --- Link to project ---
target_include_directories(peri_oled4400 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
