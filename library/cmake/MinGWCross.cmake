# SPDX-FileCopyrightText: 2018 tech4me <guiwanglong@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# Set the system name and processor
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Workaround for CMake 3.28+ MSYSTEM_PREFIX issue
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.28.0")
    set(CMAKE_SYSTEM_VERSION 10.0)
    set(CMAKE_SYSTEM_PREFIX_PATH "/usr")
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
    set(CMAKE_CROSSCOMPILING_EMULATOR "")
    set(CMAKE_CROSSCOMPILING TRUE)
endif()

# Set the toolchain prefix
set(MINGW_PREFIX /usr)
set(MINGW_TOOL_PREFIX x86_64-w64-mingw32-)

# Set the compilers with full paths
find_program(MINGW_CC ${MINGW_TOOL_PREFIX}gcc
    PATHS /usr/bin /usr/local/bin /usr/x86_64-w64-mingw32/bin
    REQUIRED)
find_program(MINGW_CXX ${MINGW_TOOL_PREFIX}g++
    PATHS /usr/bin /usr/local/bin /usr/x86_64-w64-mingw32/bin
    REQUIRED)
find_program(MINGW_WINDRES ${MINGW_TOOL_PREFIX}windres
    PATHS /usr/bin /usr/local/bin /usr/x86_64-w64-mingw32/bin
    REQUIRED)

set(CMAKE_C_COMPILER ${MINGW_CC})
set(CMAKE_CXX_COMPILER ${MINGW_CXX})
set(CMAKE_RC_COMPILER ${MINGW_WINDRES})

# Set the pkg-config executable
find_program(MINGW_PKGCONFIG ${MINGW_TOOL_PREFIX}pkg-config
    PATHS /usr/bin /usr/local/bin /usr/x86_64-w64-mingw32/bin)
if(MINGW_PKGCONFIG)
    set(ENV{PKG_CONFIG} ${MINGW_PKGCONFIG})
endif()

# Set the find root path
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(SDL2_PATH /usr/x86_64-w64-mingw32)

# Set the default flags
set(CMAKE_C_FLAGS_INIT "-D_WIN32_WINNT=0x0601")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -std=gnu++17")

# Set the default link flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++ -Wl,--enable-auto-import")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++ -Wl,--enable-auto-import")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++ -Wl,--enable-auto-import")