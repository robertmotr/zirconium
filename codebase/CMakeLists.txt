cmake_minimum_required(VERSION 3.16)
project(zirconium CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Hook strategy:
#   OFF (default) = HWBP via guac (undetected)
#   ON            = inline trampoline patch of Present() in DX11 (detected)
option(ZIRCONIUM_USE_TRAMPOLINE "Use inline trampoline hook instead of HWBP" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/intermediate)
set(CMAKE_PDB_OUTPUT_DIRECTORY     ${CMAKE_SOURCE_DIR}/../build)

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp)
    message(FATAL_ERROR
        "external/imgui is missing. Run: git submodule update --init --recursive")
endif()
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/external/guac/guac.h)
    message(FATAL_ERROR
        "external/guac is missing. Run: git submodule update --init --recursive")
endif()

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

message(STATUS "zirconium: ${CMAKE_BUILD_TYPE} | ${CMAKE_CXX_COMPILER_ID} | ${CMAKE_SIZEOF_VOID_P}-byte ptrs")

# ImGui static lib
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_compile_definitions(imgui PRIVATE
    IMGUI_DEFINE_MATH_OPERATORS
    WIN32
    UNICODE
    _UNICODE
)

# Must be compiled with /Gz too, the main DLL reads imgui headers under StdCall
# mode, so the compiled definitions must match or you get LNK2019.
if(MSVC)
    target_compile_options(imgui PRIVATE /Gz /MP)
endif()

set(ZIRCONIUM_SOURCES
    src/zirconium_main.cpp
    src/zirconium_game.cpp
    src/zirconium_imgui_overlay.cpp
    src/zirconium_log.cpp
    src/zirconium_memory.cpp
)
if(ZIRCONIUM_USE_TRAMPOLINE)
    list(APPEND ZIRCONIUM_SOURCES src/zirconium_hook_dx11.cpp)
    message(STATUS "zirconium: hook strategy = trampoline")
else()
    message(STATUS "zirconium: hook strategy = HWBP (guac)")
endif()

add_library(zirconium SHARED ${ZIRCONIUM_SOURCES})

target_precompile_headers(zirconium PRIVATE includes/pch.h)

target_include_directories(zirconium PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    includes
    external/guac
)

# Per-configuration preprocessor definitions
# Debug   → mirrors Debug|Win32
# Release → mirrors Release|Win32
target_compile_definitions(zirconium PRIVATE
    WIN32
    UNICODE
    _UNICODE
    IMGUI_DEFINE_MATH_OPERATORS
    $<$<CONFIG:Debug>:_DEBUG;_CONSOLE;GUAC_DEBUG>
    $<$<CONFIG:Release>:NDEBUG;_CONSOLE>
    $<$<BOOL:${ZIRCONIUM_USE_TRAMPOLINE}>:ZIRCONIUM_USE_TRAMPOLINE>
)

# MSVC compile options
if(MSVC)
    target_compile_options(zirconium PRIVATE
        /W3             # Warning level 3
        /permissive-    # Conformance mode (matches ConformanceMode=true)
        /Gz             # Default calling convention: __stdcall
        /fp:strict      # Strict floating-point (matches FloatingPointModel=Strict)
        /GS-            # Disable buffer security check
        /guard:cf-      # Disable Control Flow Guard
        /Oi             # Enable intrinsic functions
        /MP             # Multi-processor compilation

        # Debug-specific
        $<$<CONFIG:Debug>:/Od>      # No optimization
        $<$<CONFIG:Debug>:/Zi>      # Full debug info (produces .pdb)

        # Release-specific
        $<$<CONFIG:Release>:/O2>    # Optimize for speed
        $<$<CONFIG:Release>:/Ot>    # Favor fast code
        $<$<CONFIG:Release>:/GL>    # Whole-program optimization (requires /LTCG at link)
    )

    target_link_options(zirconium PRIVATE
        /SUBSYSTEM:CONSOLE

        $<$<CONFIG:Debug>:/DEBUG>

        # Release: strip unused symbols, fold identical COMDATs, use LTCG
        $<$<CONFIG:Release>:/DEBUG>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
        $<$<CONFIG:Release>:/LTCG>
    )
endif()

target_link_libraries(zirconium PRIVATE
    imgui
    d3d11
    dxgi
    dbghelp
)