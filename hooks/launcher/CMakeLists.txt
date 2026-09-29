option(L10N_BUILD_LAUNCHERS "Build optional application launchers" OFF)

if(NOT L10N_BUILD_LAUNCHERS)
    return()
endif()

add_executable(KnaldLauncher WIN32
    KnaldLauncher.cpp
    KnaldLauncher.rc
)

set_target_properties(KnaldLauncher
    PROPERTIES
        FOLDER "hooks"
)
hook_set_dll_output(KnaldLauncher ida)

target_compile_definitions(KnaldLauncher
    PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

target_link_libraries(KnaldLauncher PRIVATE shell32)

if(MSVC)
    target_compile_options(KnaldLauncher PRIVATE /W4 /permissive-)
endif()

add_executable(WorldMachineLauncher WIN32
    KnaldLauncher.cpp
    WorldMachineLauncher.rc
)

set_target_properties(WorldMachineLauncher
    PROPERTIES
        FOLDER "hooks"
)
hook_set_dll_output(WorldMachineLauncher worldmachine)

target_compile_definitions(WorldMachineLauncher
    PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        L10N_WORLD_MACHINE_LAUNCHER=1
)

target_link_libraries(WorldMachineLauncher PRIVATE shell32)

if(MSVC)
    target_compile_options(WorldMachineLauncher PRIVATE /W4 /permissive-)
endif()
