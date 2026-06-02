message(STATUS "hal_led available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_led INTERFACE)

target_sources(hal_led INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/led.c
)

if(PICO_BOARD MATCHES "_w")
    target_link_libraries(hal_led INTERFACE
        pico_stdlib
	    hardware_gpio
    )
    if(PICO_WIFI_TCP_ENABLED OR PICO_WIFI_UDP_ENABLED)
        target_link_libraries(hal_led INTERFACE
            pico_cyw43_arch_lwip_poll
        )
    else()
        target_link_libraries(hal_led INTERFACE
            pico_cyw43_arch_none
        )
    endif()
    message(STATUS "Add LED control using CYW43")
    add_definitions(-DLED_CYW43_SUPPORTED)
    target_compile_definitions(hal_led INTERFACE LED_CYW43_SUPPORTED)
elseif(PICO_BOARD MATCHES "adafruit_kb")
    target_link_libraries(hal_led INTERFACE
        pico_stdlib
	    hardware_gpio
        peri_ws2812
    )
    message(STATUS "Add LED control using WS2812 on KB2040")
    target_compile_definitions(hal_led INTERFACE LED_KB2040_SUPPORTED)
elseif(PICO_BOARD MATCHES "tiny2040")
    target_link_libraries(hal_led INTERFACE
        pico_stdlib
	    hardware_gpio
    )
    message(STATUS "Add LED control using Tiny2040")
    target_compile_definitions(hal_led INTERFACE LED_TINY2040_SUPPORTED)
else()
    target_link_libraries(hal_led INTERFACE
        pico_stdlib
        hardware_gpio
    )
    message(STATUS "Using normal LED support")
endif()

target_include_directories(hal_led INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
