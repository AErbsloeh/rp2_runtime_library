message(STATUS "adc_ads1299 available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(adc_ads1299 INTERFACE)

target_sources(adc_ads1299 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ads1299.c
)

target_link_libraries(adc_ads1299 INTERFACE
	hardware_gpio
    hardware_spi
    hal_spi
)

target_include_directories(adc_ads1299 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
