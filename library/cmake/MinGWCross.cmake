# SPDX-FileCopyrightText: 2018 tech4me <guiwanglong@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

set(MINGW_PREFIX   /usr/x86_64-w64-mingw32/)
set(CMAKE_SYSTEM_NAME               Windows)
set(CMAKE_SYSTEM_PROCESSOR           x86_64)


set(CMAKE_FIND_ROOT_PATH            ${MINGW_PREFIX})
set(SDL2_PATH                       ${MINGW_PREFIX})
set(MINGW_TOOL_PREFIX               ${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32-)

# Specify the cross compiler
set(CMAKE_C_COMPILER            ${MINGW_TOOL_PREFIX}gcc)
set(CMAKE_CXX_COMPILER          ${MINGW_TOOL_PREFIX}g++)
set(CMAKE_RC_COMPILER           ${MINGW_TOOL_PREFIX}windres)

# Mingw tools
set(STRIP                       ${MINGW_TOOL_PREFIX}strip)
set(WINDRES                     ${MINGW_TOOL_PREFIX}windres)
set(ENV{PKG_CONFIG}             ${MINGW_TOOL_PREFIX}pkg-config)

# ccache wrapper
option(USE_CCACHE "Use ccache for compilation" OFF)
if(USE_CCACHE)
    find_program(CCACHE ccache)
    if(CCACHE)
        message(STATUS "Using ccache found in PATH")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE})
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE})
    else(CCACHE)
        message(WARNING "USE_CCACHE enabled, but no ccache found")
    endif(CCACHE)
endif(USE_CCACHE)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)


# Echo modified cmake vars to screen for debugging purposes
if(NOT DEFINED ENV{MINGW_DEBUG_INFO})
        message("")
        message("Custom cmake vars: (blank = system default)")
        message("-----------------------------------------")
        message("* CMAKE_C_COMPILER                     : ${CMAKE_C_COMPILER}")
        message("* CMAKE_CXX_COMPILER                   : ${CMAKE_CXX_COMPILER}")
        message("* CMAKE_RC_COMPILER                    : ${CMAKE_RC_COMPILER}")
        message("* WINDRES                              : ${WINDRES}")
        message("* ENV{PKG_CONFIG}                      : $ENV{PKG_CONFIG}")
        message("* STRIP                                : ${STRIP}")
        message("* USE_CCACHE                           : ${USE_CCACHE}")
        message("")
        # So that the debug info only appears once
        set(ENV{MINGW_DEBUG_INFO} SHOWN)
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