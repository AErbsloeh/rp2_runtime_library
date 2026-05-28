message(STATUS "hal_helper available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_helper INTERFACE)

target_sources(hal_helper INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/helper.c
    ${CMAKE_CURRENT_SOURCE_DIR}/crc.c
)

target_link_libraries(hal_helper INTERFACE
	pico_stdlib
)

target_include_directories(hal_helper INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)