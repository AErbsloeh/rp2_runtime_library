cmake_minimum_required(VERSION 3.13)

# Import check of the runtime library: the library is imported like in a project (local checkout or FetchContent)
# and every library target is compiled and linked into an own executable. A broken CMakeLists.txt,
# a wrong include path or a source file which does not compile is found here.
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(runtime_library_ci C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

set(CI_LIBRARY_SOURCE "local" CACHE STRING "Import of the library: local (add_subdirectory) or fetch (FetchContent)")
set(CI_LIBRARY_REPO "" CACHE STRING "Git repository for CI_LIBRARY_SOURCE=fetch")
set(CI_LIBRARY_TAG "main" CACHE STRING "Git tag, branch or commit for CI_LIBRARY_SOURCE=fetch")

option(CI_ENABLE_WIFI "Build the WiFi libraries (only for boards with CYW43)" OFF)
if(CI_ENABLE_WIFI)
    set(PICO_WIFI_TCP_ENABLED ON CACHE BOOL "" FORCE)
    set(PICO_WIFI_UDP_ENABLED ON CACHE BOOL "" FORCE)
    set(PICO_BTSTACK_ENABLED ON CACHE BOOL "" FORCE)
endif()

if(CI_LIBRARY_SOURCE STREQUAL "fetch")
    include(FetchContent)
    FetchContent_Declare(rp2_runtime_library
        GIT_REPOSITORY ${CI_LIBRARY_REPO}
        GIT_TAG ${CI_LIBRARY_TAG}
    )
    FetchContent_MakeAvailable(rp2_runtime_library)
    set(RUNTIME_LIBRARY_DIR ${rp2_runtime_library_SOURCE_DIR})
else()
    get_filename_component(RUNTIME_LIBRARY_DIR ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)
    add_subdirectory(${RUNTIME_LIBRARY_DIR} ${CMAKE_BINARY_DIR}/rp2_runtime_library)
endif()

function(collect_targets dir out_var)
    set(found "")
    get_property(targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target ${targets})
        get_target_property(type ${target} TYPE)
        if(type STREQUAL "INTERFACE_LIBRARY")
            list(APPEND found ${target})
        endif()
    endforeach()
    get_property(subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirs})
        set(sub_found "")
        collect_targets(${subdir} sub_found)
        list(APPEND found ${sub_found})
    endforeach()
    set(${out_var} ${found} PARENT_SCOPE)
endfunction()

collect_targets(${RUNTIME_LIBRARY_DIR} LIBRARY_TARGETS)
list(REMOVE_DUPLICATES LIBRARY_TARGETS)
list(SORT LIBRARY_TARGETS)

if(NOT CI_ENABLE_WIFI)
    list(FILTER LIBRARY_TARGETS EXCLUDE REGEX "^hal_wifi_|^hal_transport_wifi$|^hal_transport_bt$|^hal_btspp$")
endif()
message(STATUS "Library targets: ${LIBRARY_TARGETS}")

foreach(lib ${LIBRARY_TARGETS})
    add_executable(ci_${lib} ${CMAKE_CURRENT_LIST_DIR}/main.c)
    target_link_libraries(ci_${lib} pico_stdlib ${lib})
    if(CI_ENABLE_WIFI)
        # Files which a project has to provide for the WiFi libraries (lwipopts.h, transport_wifi_config.h)
        target_include_directories(ci_${lib} PRIVATE ${RUNTIME_LIBRARY_DIR}/hal/wifi ${CMAKE_CURRENT_LIST_DIR})
    endif()
endforeach()
