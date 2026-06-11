cmake_minimum_required(VERSION 3.20)
project(InternalBoot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-only application
if(NOT WIN32)
    message(WARNING "InternalBoot is a Windows-only application. Cross-compilation requires MinGW-w64.")
endif()

set(SOURCES
    src/main.cpp
    src/utils.cpp
    src/theme.cpp
    src/iso.cpp
    src/disk.cpp
    src/bcd.cpp
    src/boot.cpp
    src/appstate.cpp
    src/wizard.cpp
)

set(HEADERS
    src/utils.h
    src/theme.h
    src/iso.h
    src/disk.h
    src/bcd.h
    src/boot.h
    src/appstate.h
    src/wizard.h
)

add_executable(InternalBoot WIN32 ${SOURCES} ${HEADERS})

target_link_libraries(InternalBoot PRIVATE
    comctl32
    gdi32
    gdiplus
    shell32
    shlwapi
    uxtheme
)

# For MSVC: enable Unicode and static runtime
if(MSVC)
    target_compile_definitions(InternalBoot PRIVATE UNICODE _UNICODE)
    set_property(TARGET InternalBoot PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# For MinGW
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(InternalBoot PRIVATE UNICODE _UNICODE)
    target_compile_options(InternalBoot PRIVATE -pthread)
    target_link_options(InternalBoot PRIVATE -pthread)
endif()
