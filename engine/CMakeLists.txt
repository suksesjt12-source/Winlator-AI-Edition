cmake_minimum_required(VERSION 3.18)
project(aiedition_engine_core LANGUAGES CXX)

add_library(aie_execution_core STATIC
    src/pe_image.cpp
    src/pe_loader.cpp
    src/guest_memory.cpp
    src/x64_interpreter.cpp
)
target_include_directories(aie_execution_core PUBLIC include)
target_compile_features(aie_execution_core PUBLIC cxx_std_17)
target_compile_options(aie_execution_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(pe_image_tests tests/pe_image_tests.cpp)
target_link_libraries(pe_image_tests PRIVATE aie_execution_core)

add_executable(pe_loader_tests tests/pe_loader_tests.cpp)
target_link_libraries(pe_loader_tests PRIVATE aie_execution_core)

add_executable(x64_interpreter_tests tests/x64_interpreter_tests.cpp)

add_executable(guest_memory_tests tests/guest_memory_tests.cpp)
target_link_libraries(guest_memory_tests PRIVATE aie_execution_core)
target_link_libraries(x64_interpreter_tests PRIVATE aie_execution_core)

enable_testing()
add_test(NAME pe_image_tests COMMAND pe_image_tests)
add_test(NAME pe_loader_tests COMMAND pe_loader_tests)
add_test(NAME x64_interpreter_tests COMMAND x64_interpreter_tests)
add_test(NAME guest_memory_tests COMMAND guest_memory_tests)
