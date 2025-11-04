cmake_minimum_required(VERSION 3.20.0)

project(mt_uhf_sdk_c
    HOMEPAGE_URL "metratec.com"
    VERSION "0.3.1"
    DESCRIPTION "Access to metratec's second generation UHF RFID readers in C"
    LANGUAGES C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED CMAKE_BUILD_TYPE OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Setting to default CMAKE_BUILD_TYPE : ${CMAKE_BUILD_TYPE}")
endif()

# Handle device setting
set(SUPPORTED_DEVICES "PLRM" "PULSAR_LR" "QRG2" "DESKID_UHF_V2")
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "MT_UHF_DEVICE_TYPE is '${MT_UHF_DEVICE_TYPE}'")

if(DEFINED MT_UHF_DEVICE_TYPE)
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

# A longer list needs memory and runtime, can be shortened if the last bytes are needed
if(NOT MT_UHF_MAX_EVENT_TYPES)
    set(MT_UHF_MAX_EVENT_TYPES 10)
endif()

# Use the internal assert function so the user needs to supply a handler
if(NOT MT_UHF_USE_ASSERT)
    set(MT_UHF_USE_ASSERT 0)
endif()

# Memory size of the response buffer
if(NOT MT_UHF_SETTING_RXBUFFER_SIZE)
    set(MT_UHF_SETTING_RXBUFFER_SIZE 1024)
endif()

# Buffer size for a single answer line
if(NOT MT_UHF_PARSER_BUF_SIZE)
    set(MT_UHF_PARSER_BUF_SIZE 256)
endif()

if(${MT_UHF_PARSER_BUF_SIZE} GREATER_EQUAL ${MT_UHF_SETTING_RXBUFFER_SIZE})
    message(WARNING "Parser buffer can't be fully used as receive buffer isn't as big")
endif()

add_compile_definitions(MT_UHF_SETTING_RXBUFFER_SIZE=${MT_UHF_SETTING_RXBUFFER_SIZE}
    MT_UHF_MAX_EVENT_TYPES=${MT_UHF_MAX_EVENT_TYPES}
    MT_UHF_PARSER_BUF_SIZE=${MT_UHF_PARSER_BUF_SIZE}
    MT_UHF_USE_ASSERT=${MT_UHF_USE_ASSERT}
)

if(DEFINED _DEBUG_PRINTOUT_PRINTF)
    add_compile_definitions(DEBUG_PRINTOUT_PRINTF=${_DEBUG_PRINTOUT_PRINTF})
endif()

# Library
set(mt_device_include_path ${CMAKE_CURRENT_SOURCE_DIR}/code/include/metratec/uhf_reader/device_type/${MT_UHF_DEVICE_TYPE_LOWER}.h)
set(mt_device_include_path ${CMAKE_CURRENT_SOURCE_DIR}/code/include/metratec/uhf_reader/device_type/${MT_UHF_DEVICE_TYPE_LOWER}.h PARENT_SCOPE)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/code/include)
add_subdirectory(code)

if(DEFINED EXAMPLE_POSIX_BASIC)
    if (EXAMPLE_POSIX_BASIC)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples/posix_basic)
endif()
endif()

if(DEFINED EXAMPLE_POSIX_TEST)
    if(EXAMPLE_POSIX_TEST)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples/posix_test)
    endif()
endif()
