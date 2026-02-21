cmake_minimum_required(VERSION 3.16)
project(zirconium CXX)

# Language standard 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/intermediate)
set(CMAKE_PDB_OUTPUT_DIRECTORY     ${CMAKE_SOURCE_DIR}/../build)

# ImGui static lib
# Must be compiled with /Gz too, the main DLL reads imgui headers under StdCall
# mode, so the compiled definitions must match or you get LNK2019.
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

if(MSVC)
    target_compile_options(imgui PRIVATE /Gz /MP)
endif()

add_library(zirconium SHARED
    src/dll_main.cpp
    src/globals.cpp
    src/hooks/hook_dx11.cpp
    src/rendering/imgui_overlay.cpp
    src/utils/logger.cpp
    src/utils/mem.cpp
)

target_include_directories(zirconium PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    includes
    includes/hooks
    includes/utils
    includes/rendering
)

# Per-configuration preprocessor definitions
# Debug   → mirrors Debug|Win32   (DLL, MENU_ONLY defined)
# Release → mirrors Release|Win32
target_compile_definitions(zirconium PRIVATE
    WIN32
    UNICODE
    _UNICODE
    IMGUI_DEFINE_MATH_OPERATORS
    $<$<CONFIG:Debug>:_DEBUG;_CONSOLE;MENU_ONLY>
    $<$<CONFIG:Release>:NDEBUG;_CONSOLE>
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

# ── System libraries ───────────────────────────────────────────────────────────
# d3d11/dxgi/dbghelp are also referenced via #pragma comment in pch.h;
# explicit linking here makes the dependency visible and portable.
target_link_libraries(zirconium PRIVATE
    imgui
    d3d11
    dxgi
    dbghelp
)

# ── Notes ──────────────────────────────────────────────────────────────────────
# MenuOnly|Win32 (standalone exe with WinMainCRTStartup entry point):
#   If you want a second target for the standalone menu app, add another
#   add_executable() here with:
#     target_compile_definitions(... _MENU_ONLY ...)
#     target_link_options(... /ENTRY:WinMainCRTStartup ...)
#
# Legacy DirectX SDK (DXSDK_DIR):
#   The MenuOnly|Win32 config referenced $(DXSDK_DIR)Include and $(DXSDK_DIR)Lib/x86.
#   On Windows 10 SDK these headers ship in-box, so this is usually not needed.
#   If you hit missing dx headers, set DXSDK_DIR in your environment and add:
#     target_include_directories(zirconium PRIVATE $ENV{DXSDK_DIR}/Include)
#     link_directories($ENV{DXSDK_DIR}/Lib/x86)
