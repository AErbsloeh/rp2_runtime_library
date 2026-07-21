message(STATUS "adc_ad4858 available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(adc_ad4858 INTERFACE)

target_sources(adc_ad4858 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ad4858.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ad4858_cmos.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ad4858_spi.c
)

target_link_libraries(adc_ad4858 INTERFACE
    pico_stdlib
	hardware_gpio
	hardware_pwm
	hardware_clocks
	hardware_irq
    hal_spi
)

target_include_directories(adc_ad4858 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
