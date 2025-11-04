cmake_minimum_required(VERSION 3.20.0)

message(STATUS "SDK Build type: '${CMAKE_BUILD_TYPE}'")

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message(STATUS "Debug flags: '${CMAKE_C_FLAGS_DEBUG}'")
else()
    message(STATUS "RFID SDK C Flags: '${CMAKE_C_FLAGS}'")
endif()

add_library(mt_uhf_sdk_c STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/select_helper.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/inventory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rw.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api.c

    ${CMAKE_CURRENT_SOURCE_DIR}/src/intern/parse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/intern/framehandler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/intern/receive_buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/intern/reader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/intern/assert.c)
target_precompile_headers(mt_uhf_sdk_c PUBLIC ${mt_device_include_path})

target_compile_options(mt_uhf_sdk_c PUBLIC -g3)
target_compile_features(mt_uhf_sdk_c PUBLIC c_std_99)
target_include_directories(mt_uhf_sdk_c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
add_compile_definitions(mt_uhf_sdk_c
    MT_UHF_SETTING_RXBUFFER_SIZE=${MT_UHF_SETTING_RXBUFFER_SIZE}
    MT_UHF_MAX_EVENT_TYPES=${MT_UHF_MAX_EVENT_TYPES}
    MT_UHF_PARSER_BUF_SIZE=${MT_UHF_PARSER_BUF_SIZE}
    MT_UHF_USE_ASSERT=${MT_UHF_USE_ASSERT}
)

# if(TARGET_GROUP STREQUAL test)
# include(CTest)
# add_subdirectory(tests)
# endif()
