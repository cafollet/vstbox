cmake_minimum_required(VERSION 3.20)
project(vstbox_native VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(vstbox_benchmark_core STATIC
    src/synthetic_processor.cpp
)
target_include_directories(vstbox_benchmark_core PUBLIC include)

if(MSVC)
    target_compile_options(vstbox_benchmark_core PRIVATE /W4)
else()
    target_compile_options(vstbox_benchmark_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(vstbox_synthetic_bench src/synthetic_bench.cpp)
target_link_libraries(vstbox_synthetic_bench PRIVATE vstbox_benchmark_core)

# The real VST3 adapter will plug into the same AudioProcessor interface.
# We intentionally do not vendor the Steinberg SDK in this repository.
option(VSTBOX_ENABLE_VST3 "Build the VST3 benchmark adapter when an SDK is supplied" OFF)
if(VSTBOX_ENABLE_VST3)
    if(NOT DEFINED VST3_SDK_ROOT)
        message(FATAL_ERROR "VSTBOX_ENABLE_VST3=ON requires -DVST3_SDK_ROOT=/path/to/vst3sdk")
    endif()
    message(STATUS "VST3 SDK requested at: ${VST3_SDK_ROOT}")
    message(FATAL_ERROR "VST3 adapter scaffold is reserved for the next implementation step; synthetic benchmark core is ready.")
endif()
