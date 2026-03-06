message(STATUS "sens_pac193x available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(sens_pac193x INTERFACE)

target_sources(sens_pac193x INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/pac193x.c
)

target_link_libraries(sens_pac193x INTERFACE
    pico_stdlib
	hal_i2c
)

target_include_directories(sens_pac193x INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
