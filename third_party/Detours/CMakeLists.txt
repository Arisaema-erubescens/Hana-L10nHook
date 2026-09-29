add_library(detours STATIC
    src/creatwth.cpp
    src/detours.cpp
    src/detours.h
    src/detver.h
    src/disasm.cpp
    src/disolarm.cpp
    src/disolarm64.cpp
    src/disolia64.cpp
    src/disolx64.cpp
    src/disolx86.cpp
    src/image.cpp
    src/modules.cpp
)

set_target_properties(detours PROPERTIES FOLDER "libraries")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    src/creatwth.cpp
    src/detours.cpp
    src/detours.h
    src/detver.h
    src/disasm.cpp
    src/disolarm.cpp
    src/disolarm64.cpp
    src/disolia64.cpp
    src/disolx64.cpp
    src/disolx86.cpp
    src/image.cpp
    src/modules.cpp
)

target_include_directories(detours
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(detours
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0501
        _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(detours PRIVATE /W4 /Zc:gotoScope-)
endif()
