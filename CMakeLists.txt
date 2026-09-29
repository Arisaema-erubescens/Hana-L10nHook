cmake_minimum_required(VERSION 3.20)

project(Hook LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# The host bundles an older MSVC runtime. Link the hook runtime statically so
# STL calls never bind to the host's incompatible MSVCP140.dll.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT WIN32)
    message(FATAL_ERROR "This project targets Windows only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(HOOK_TARGET_ARCH x64)
else()
    set(HOOK_TARGET_ARCH x86)
endif()

set(HOOK_PROXY_NAMES "version;winmm" CACHE STRING "DLL proxy names to build.")

function(hook_set_dll_output target project_name)
    set(output_dir ${CMAKE_SOURCE_DIR}/build/bin/${project_name}/${HOOK_TARGET_ARCH})

    set_target_properties(${target}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${output_dir}
            LIBRARY_OUTPUT_DIRECTORY ${output_dir}
    )

    foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${config} config_upper)
        set_target_properties(${target}
            PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_${config_upper} ${output_dir}
                LIBRARY_OUTPUT_DIRECTORY_${config_upper} ${output_dir}
        )
    endforeach()
endfunction()

add_subdirectory(third_party/Detours)
add_subdirectory(common)
add_subdirectory(hooks)
