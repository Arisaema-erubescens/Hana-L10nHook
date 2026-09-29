add_library(common STATIC
    include/AddressResolver.h
    include/DetoursHook.h
    include/Encoding.h
    include/Logger.h
    include/ModuleUtils.h
    include/QStringBridge.h
    include/StringUtils.h
    include/TranslationManager.h
    src/AddressResolver.cpp
    src/DetoursHook.cpp
    src/Encoding.cpp
    src/Logger.cpp
    src/ModuleUtils.cpp
    src/QStringBridge.cpp
    src/StringUtils.cpp
    src/TranslationManager.cpp
)

set_target_properties(common PROPERTIES FOLDER "libraries")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    include/AddressResolver.h
    include/DetoursHook.h
    include/Encoding.h
    include/Logger.h
    include/ModuleUtils.h
    include/QStringBridge.h
    include/StringUtils.h
    include/TranslationManager.h
    src/AddressResolver.cpp
    src/DetoursHook.cpp
    src/Encoding.cpp
    src/Logger.cpp
    src/ModuleUtils.cpp
    src/QStringBridge.cpp
    src/StringUtils.cpp
    src/TranslationManager.cpp
)

target_include_directories(common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(common
    PUBLIC
        detours
)

target_compile_definitions(common
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

if(MSVC)
    target_compile_options(common PRIVATE /W4 /permissive-)
endif()
