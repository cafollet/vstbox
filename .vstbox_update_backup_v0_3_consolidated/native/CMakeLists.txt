cmake_minimum_required(VERSION 3.20)
project(vstbox_native VERSION 0.3.0 LANGUAGES CXX)

# On macOS, Steinberg's module loader and common helpers contain
# Objective-C++ sources. Enable OBJCXX at top-level project scope,
# before the VST3 SDK or any dependent targets are added.
if(APPLE)
    enable_language(OBJCXX)
endif()

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

# Real VST3 benchmarking is optional so the base simulator remains buildable
# without downloading a third-party SDK. The SDK is supplied from external/.
option(VSTBOX_ENABLE_VST3 "Build the VST3 benchmark adapter when an SDK is supplied" OFF)
if(VSTBOX_ENABLE_VST3)
    if(NOT DEFINED VST3_SDK_ROOT)
        message(FATAL_ERROR "VSTBOX_ENABLE_VST3=ON requires -DVST3_SDK_ROOT=/path/to/vst3sdk")
    endif()
    if(NOT EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
        message(FATAL_ERROR "VST3_SDK_ROOT does not look like a VST3 SDK checkout: ${VST3_SDK_ROOT}")
    endif()

    # Keep Steinberg examples/wrappers out of this build. We only need the
    # hosting helper libraries and interfaces for the VSTBox offline harness.
    set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_AUV2_BUILDS OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_AUV3_BUILDS OFF CACHE BOOL "" FORCE)

    add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/steinberg-vst3sdk" EXCLUDE_FROM_ALL)

    add_library(vstbox_vst3_adapter STATIC
        src/vst3_processor.cpp
    )
    target_include_directories(vstbox_vst3_adapter PUBLIC include "${VST3_SDK_ROOT}")
    target_link_libraries(vstbox_vst3_adapter PUBLIC sdk_hosting)

    # Steinberg's sdk_hosting contains the platform-independent hosting code,
    # but Module::create() is implemented in a platform-specific source file.
    if(APPLE)
        target_sources(vstbox_vst3_adapter PRIVATE
            "${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm"
        )

        # Steinberg's macOS module loader requires Objective-C ARC.
        # This mirrors the SDK's own audiohost build configuration.
        set_source_files_properties(
            "${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm"
            PROPERTIES
            COMPILE_FLAGS "-fobjc-arc"
        )
        target_link_libraries(vstbox_vst3_adapter PUBLIC
            "-framework CoreFoundation"
            "-framework Foundation"
        )
    elseif(WIN32)
        target_sources(vstbox_vst3_adapter PRIVATE
            "${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_win32.cpp"
        )
    elseif(UNIX)
        target_sources(vstbox_vst3_adapter PRIVATE
            "${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_linux.cpp"
        )
        target_link_libraries(vstbox_vst3_adapter PUBLIC "${CMAKE_DL_LIBS}")
    endif()

    if(MSVC)
        target_compile_options(vstbox_vst3_adapter PRIVATE /W4)
    else()
        target_compile_options(vstbox_vst3_adapter PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_executable(vstbox_vst3_bench src/vst3_bench.cpp)
    target_link_libraries(vstbox_vst3_bench PRIVATE vstbox_vst3_adapter)
endif()
