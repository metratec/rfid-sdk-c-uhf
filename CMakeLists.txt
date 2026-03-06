cmake_minimum_required(VERSION 3.20.0)

project(mt_uhf_sdk_c
    HOMEPAGE_URL "metratec.com"
    VERSION "0.6"
    DESCRIPTION "Access to metratec's second generation UHF RFID readers in C"
    LANGUAGES C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED CMAKE_BUILD_TYPE OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Setting to default CMAKE_BUILD_TYPE : ${CMAKE_BUILD_TYPE}")
endif()

# Handle device setting
set(SUPPORTED_DEVICES "PLRM" "QRG2_ETSI" "QRG2_FCC") # Untested devices: "PULSAR_LR" "DESKID_UHF_V2_ETSI" "DESKID_UHF_V2_FCC"
set(CMAKE_CXX_EXTENSIONS OFF)

set(MT_UHF_DEVICE_TYPE CACHE STRING "One of ${SUPPORTED_DEVICES}")

if(MT_UHF_DEVICE_TYPE)
    string(TOUPPER ${MT_UHF_DEVICE_TYPE} MT_UHF_DEVICE_TYPE_UPPER)
    string(TOLOWER ${MT_UHF_DEVICE_TYPE} MT_UHF_DEVICE_TYPE_LOWER)

    if(NOT MT_UHF_DEVICE_TYPE_UPPER IN_LIST SUPPORTED_DEVICES)
        message(FATAL_ERROR "Device type is '${MT_UHF_DEVICE_TYPE}' but must be one of ${SUPPORTED_DEVICES}")
    endif()

    message(STATUS "Device type used is: ${MT_UHF_DEVICE_TYPE_UPPER}")
else()
    message(FATAL_ERROR "Device type is missing, add '-DMT_UHF_DEVICE_TYPE=[DEVICENAME]'")
endif()

# handle library feature / buffer settings

set(MT_UHF_MAX_EVENT_TYPES 10
    CACHE STRING "A longer list needs memory and runtime, can be shortened if the last bytes are needed")

set(MT_UHF_USE_ASSERT OFF
    CACHE BOOL "Use the internal assert function so the user needs to supply a handler")
if(MT_UHF_USE_ASSERT)
    add_compile_definitions(MT_UHF_USE_ASSERT=1)
endif()

set(MT_UHF_SETTING_RXBUFFER_SIZE 1024
    CACHE STRING "Memory size of the response buffer")

set(MT_UHF_PARSER_BUF_SIZE 256
    CACHE STRING "Buffer size for a single answer line")

if(${MT_UHF_PARSER_BUF_SIZE} GREATER_EQUAL ${MT_UHF_SETTING_RXBUFFER_SIZE})
    message(WARNING "Parser buffer can't be fully used as receive buffer isn't as big")
endif()

# FIXME: You would usually use configure_file() to generate a config.h.
add_compile_definitions(MT_UHF_SETTING_RXBUFFER_SIZE=${MT_UHF_SETTING_RXBUFFER_SIZE}
    MT_UHF_MAX_EVENT_TYPES=${MT_UHF_MAX_EVENT_TYPES}
    MT_UHF_PARSER_BUF_SIZE=${MT_UHF_PARSER_BUF_SIZE}
)

set(_DEBUG_PRINTOUT_PRINTF OFF
    CACHE BOOL "Enable some additional debug logging to stdout")
if(_DEBUG_PRINTOUT_PRINTF)
    add_compile_definitions(DEBUG_PRINTOUT_PRINTF=1)
endif()

# Library
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/code/include)
add_subdirectory(code)

# Examples
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples/posix_shared)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples/basic_inventory)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples/function_test)

