message(STATUS "sens_pac195x available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(sens_pac195x INTERFACE)

target_sources(sens_pac193x INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/pac195x.c
)

target_link_libraries(sens_pac195x INTERFACE
    pico_stdlib
	hal_i2c
)

target_include_directories(sens_pac195x INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
