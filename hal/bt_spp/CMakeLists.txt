message(STATUS "hal_btspp available at: ${CMAKE_CURRENT_LIST_FILE}")
add_library(hal_btspp INTERFACE)

# Raspberry Pi Radio Module 2 (RM2, CYW43439) on a board without own CYW43 (e.g. pico, pico2): pins of the module (GPIO numbers).
# DATA is the shared data pin (DIO) of the module. The project has to set PICO_CYW43_SUPPORTED to 1 before pico_sdk_init().
# Boards with CYW43 (pico_w, pico2_w, pimoroni_pico_plus2_w_rp2350, ...) need none of these settings.
set(BT_RM2_PIN_WL_REG_ON "" CACHE STRING "RM2: GPIO of WL_ON / REG_ON")
set(BT_RM2_PIN_WL_DATA "" CACHE STRING "RM2: GPIO of the data pin (DIO)")
set(BT_RM2_PIN_WL_CLOCK "" CACHE STRING "RM2: GPIO of the SPI clock (CLK)")
set(BT_RM2_PIN_WL_CS "" CACHE STRING "RM2: GPIO of the chip select (CS)")
set(BT_RM2_PIO_CLOCK_DIV_INT "2" CACHE STRING "RM2: integer clock divider of the PIO SPI (use 3 if the module does not start)")

if(PICO_BTSTACK_ENABLED)
    target_sources(hal_btspp INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/bl_spp.c
    )
    target_link_libraries(hal_btspp INTERFACE
        pico_stdlib
        pico_btstack_classic
        pico_btstack_cyw43
        pico_cyw43_arch_none
    )
    # btstack_config.h is found by the BTstack sources through this folder
    target_include_directories(hal_btspp INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if(NOT "${BT_RM2_PIN_WL_REG_ON}" STREQUAL "" AND NOT "${BT_RM2_PIN_WL_DATA}" STREQUAL ""
       AND NOT "${BT_RM2_PIN_WL_CLOCK}" STREQUAL "" AND NOT "${BT_RM2_PIN_WL_CS}" STREQUAL "")
        message(STATUS "hal_btspp: Raspberry Pi Radio Module 2 with REG_ON=${BT_RM2_PIN_WL_REG_ON}, DATA=${BT_RM2_PIN_WL_DATA}, CLOCK=${BT_RM2_PIN_WL_CLOCK}, CS=${BT_RM2_PIN_WL_CS}")
        target_compile_definitions(hal_btspp INTERFACE
            CYW43_DEFAULT_PIN_WL_REG_ON=${BT_RM2_PIN_WL_REG_ON}
            CYW43_DEFAULT_PIN_WL_DATA_OUT=${BT_RM2_PIN_WL_DATA}
            CYW43_DEFAULT_PIN_WL_DATA_IN=${BT_RM2_PIN_WL_DATA}
            CYW43_DEFAULT_PIN_WL_HOST_WAKE=${BT_RM2_PIN_WL_DATA}
            CYW43_DEFAULT_PIN_WL_CLOCK=${BT_RM2_PIN_WL_CLOCK}
            CYW43_DEFAULT_PIN_WL_CS=${BT_RM2_PIN_WL_CS}
            CYW43_PIO_CLOCK_DIV_INT=${BT_RM2_PIO_CLOCK_DIV_INT}
            CYW43_PIO_CLOCK_DIV_FRAC=0
        )
    endif()
endif()

target_include_directories(hal_btspp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
