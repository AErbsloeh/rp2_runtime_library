# WIFI TCP
message(STATUS "hal_wifi_tcp available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_wifi_tcp INTERFACE)

if(PICO_WIFI_TCP_ENABLED)
    message(STATUS "Copy lwipopts.h file")
    file(COPY 
        ${CMAKE_CURRENT_SOURCE_DIR}/lwipopts.h
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    )
    target_sources(hal_wifi_tcp INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/wifi_tcp.c
    )
    target_link_libraries(hal_wifi_tcp INTERFACE
        pico_stdlib
        hardware_gpio
		pico_cyw43_arch_lwip_poll
    )
endif()

target_include_directories(hal_wifi_tcp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)

# WIFI UDP
message(STATUS "hal_wifi_udp available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_wifi_udp INTERFACE)

if(PICO_WIFI_UDP_ENABLED)
    message(STATUS "Copy lwipopts.h file")
    file(COPY 
        ${CMAKE_CURRENT_SOURCE_DIR}/lwipopts.h
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    )    
    target_sources(hal_wifi_udp INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/wifi_udp.c
    )
    target_link_libraries(hal_wifi_udp INTERFACE
        pico_stdlib
	    hardware_gpio
		pico_cyw43_arch_none
    )
endif()

target_include_directories(hal_wifi_udp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}
)
