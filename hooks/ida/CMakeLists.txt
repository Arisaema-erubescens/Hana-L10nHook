set(IDA_HOOK_SOURCES
    dllmain.cpp
    Hook.h
    Hook.cpp
    Win32GdiHook.h
    Win32GdiHook.cpp
)

foreach(proxy_name IN LISTS HOOK_PROXY_NAMES)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(proxy_source ${CMAKE_SOURCE_DIR}/proxy/${proxy_name}_x64.cpp)
    else()
        set(proxy_source ${CMAKE_SOURCE_DIR}/proxy/${proxy_name}_x86.cpp)
    endif()

    if(NOT EXISTS ${proxy_source})
        message(FATAL_ERROR "Missing proxy source for ${proxy_name}: ${proxy_source}")
    endif()

    set(target_name ida_${proxy_name})
    add_library(${target_name} SHARED
        ${IDA_HOOK_SOURCES}
        ${proxy_source}
    )

    set_target_properties(${target_name}
        PROPERTIES
            OUTPUT_NAME ${proxy_name}
            FOLDER "hooks/ida"
    )
    hook_set_dll_output(${target_name} ida)

    target_link_libraries(${target_name}
        PRIVATE
            common
            detours
            user32
    )

    target_compile_definitions(${target_name}
        PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
    )

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    endif()
endforeach()

# Standalone Qt hook for applications that do not load a proxy DLL.
set(KNALD_HOOK_TARGET knald_hook)
add_library(${KNALD_HOOK_TARGET} SHARED
    ${IDA_HOOK_SOURCES}
)

set_target_properties(${KNALD_HOOK_TARGET}
    PROPERTIES
        OUTPUT_NAME KnaldHook
        FOLDER "hooks/ida"
)
hook_set_dll_output(${KNALD_HOOK_TARGET} ida)

target_link_libraries(${KNALD_HOOK_TARGET}
    PRIVATE
        common
        detours
        user32
)

target_compile_definitions(${KNALD_HOOK_TARGET}
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        XPROXY_KNALD_ONLY=1
)

if(MSVC)
    target_compile_options(${KNALD_HOOK_TARGET} PRIVATE /W4 /permissive-)
endif()

add_custom_command(TARGET ${KNALD_HOOK_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${KNALD_HOOK_TARGET}>/Dictionaries"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Dictionaries/translations.txt"
        "$<TARGET_FILE_DIR:${KNALD_HOOK_TARGET}>/Dictionaries/translations.txt"
)

set(WORLD_MACHINE_HOOK_TARGET world_machine_hook)
add_library(${WORLD_MACHINE_HOOK_TARGET} SHARED
    ${IDA_HOOK_SOURCES}
)

set_target_properties(${WORLD_MACHINE_HOOK_TARGET}
    PROPERTIES
        OUTPUT_NAME WorldMachineHook
        FOLDER "hooks/ida"
)
hook_set_dll_output(${WORLD_MACHINE_HOOK_TARGET} worldmachine)

target_link_libraries(${WORLD_MACHINE_HOOK_TARGET}
    PRIVATE
        common
        detours
        user32
)

target_compile_definitions(${WORLD_MACHINE_HOOK_TARGET}
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        XPROXY_WORLDMACHINE_ONLY=1
)

if(MSVC)
    target_compile_options(${WORLD_MACHINE_HOOK_TARGET} PRIVATE /W4 /permissive-)
endif()

add_custom_command(TARGET ${WORLD_MACHINE_HOOK_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${WORLD_MACHINE_HOOK_TARGET}>/Dictionaries"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/hooks/worldmachine/Dictionaries/translations.txt"
        "$<TARGET_FILE_DIR:${WORLD_MACHINE_HOOK_TARGET}>/Dictionaries/translations.txt"
)
