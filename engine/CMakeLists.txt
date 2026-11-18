cmake_minimum_required(VERSION 3.18)
project(aiedition_engine_core LANGUAGES CXX)

add_library(aie_pe_loader STATIC src/pe_image.cpp)
target_include_directories(aie_pe_loader PUBLIC include)
target_compile_features(aie_pe_loader PUBLIC cxx_std_17)
target_compile_options(aie_pe_loader PRIVATE -Wall -Wextra -Wpedantic)

add_executable(pe_image_tests tests/pe_image_tests.cpp)
target_link_libraries(pe_image_tests PRIVATE aie_pe_loader)
enable_testing()
add_test(NAME pe_image_tests COMMAND pe_image_tests)
