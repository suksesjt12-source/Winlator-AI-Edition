add_library(aie_execution_engine SHARED
    jni_bridge.cpp
    src/pe_image.cpp
    src/pe_loader.cpp
    src/guest_memory.cpp
    src/x64_interpreter.cpp
)

target_include_directories(aie_execution_engine PRIVATE include)
target_compile_features(aie_execution_engine PRIVATE cxx_std_17)
target_compile_options(aie_execution_engine PRIVATE -Wall -Wextra -Wpedantic)
