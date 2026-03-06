cmake_minimum_required(VERSION 3.20.0)

set(example_name "sample_function_test_${MT_UHF_DEVICE_TYPE_UPPER}")
add_executable(${example_name})
target_sources(${example_name} PUBLIC 
    main.c
    testing_functions.c
    test_continious.c)

target_link_libraries(${example_name} PUBLIC posix_with_serial)
target_link_libraries(${example_name} PUBLIC mt_uhf_sdk_c)
target_include_directories(${example_name} PUBLIC include)

set_property(TARGET ${example_name} PROPERTY C_STANDARD 99)
target_compile_options(${example_name} PUBLIC -g3)
add_compile_definitions(${example_name} MT_UHF_USE_ASSERT=${MT_UHF_USE_ASSERT})