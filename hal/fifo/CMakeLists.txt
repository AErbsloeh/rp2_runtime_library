message(STATUS "hal_fifo available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_fifo INTERFACE)

target_sources(hal_fifo INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/fifo.c
)

target_link_libraries(hal_fifo INTERFACE
    pico_stdlib
)

target_include_directories(hal_fifo INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
