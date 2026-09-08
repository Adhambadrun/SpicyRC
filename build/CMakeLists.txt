cmake_minimum_required(VERSION 3.20)
project(SpicyLamar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(SpicyLamarAutoAnswer WIN32
    ../src/main.cpp
    ../resources/app.rc
)

set_target_properties(SpicyLamarAutoAnswer PROPERTIES
    OUTPUT_NAME "SpicyLamar"
    # Single window 980x620 integrated — no detached keypad HWND
)

target_compile_definitions(SpicyLamarAutoAnswer PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    SPICY_LAMAR_TURBO
)

target_compile_options(SpicyLamarAutoAnswer PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_link_libraries(SpicyLamarAutoAnswer PRIVATE
    kernel32
    user32
    gdi32
    comctl32
    shell32
    ole32
    oleaut32
    advapi32
    uxtheme
    winmm
    avrt
    dwmapi
    uiautomationcore
    oleacc
    tdh
    psapi
)

if(MSVC)
    set_target_properties(SpicyLamarAutoAnswer PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()
