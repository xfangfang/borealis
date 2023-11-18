cmake_minimum_required(VERSION 3.10)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
set(BOREALIS_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(BOREALIS_SOURCE)
set(BOREALIS_SRC)
set(BOREALIS_INCLUDE)
set(BRLS_PLATFORM_LIBS)
set(BRLS_PLATFORM_OPTION)
if (NOT BRLS_RESOURCES_DIR)
    set(BRLS_RESOURCES_DIR ".")
endif ()

# fmt
if (USE_SYSTEM_FMT)
  find_package(fmt REQUIRED)
else()
  add_subdirectory(${BOREALIS_PATH}/lib/extern/fmt EXCLUDE_FROM_ALL)
endif()

# tweeny
if (USE_SYSTEM_TWEENY)
  find_package(Tweeny REQUIRED)
else()
  add_subdirectory(${BOREALIS_PATH}/lib/extern/tweeny EXCLUDE_FROM_ALL)
endif()

# yoga
add_subdirectory(${BOREALIS_PATH}/lib/extern/yoga/yoga EXCLUDE_FROM_ALL)
target_compile_options(yogacore PRIVATE -fvisibility=default)

# opencc
if (BRLS_USE_OPENCC)
    list(APPEND BRLS_PLATFORM_LIBS libopencc)
    list(APPEND BOREALIS_INCLUDE ${BRLS_OPENCC_INCLUDE})
    list(APPEND BRLS_PLATFORM_OPTION -DOPENCC)
endif ()

# boost::filesystem
if (USE_BOOST_FILESYSTEM)
    find_package(Boost 1.44 REQUIRED COMPONENTS filesystem)
    if(Boost_FOUND)
        list(APPEND BRLS_PLATFORM_LIBS Boost::filesystem)
        list(APPEND BRLS_PLATFORM_OPTION -DUSE_BOOST_FILESYSTEM)
    endif()
endif ()

# libromfs
if(USE_LIBROMFS)
    add_subdirectory(${BOREALIS_PATH}/lib/extern/libromfs EXCLUDE_FROM_ALL)
    list(APPEND BRLS_PLATFORM_LIBS ${LIBROMFS_LIBRARY})
endif()

# tinyxml2
if (USE_SYSTEM_TINYXML2)
    find_package(tinyxml2 REQUIRED COMPONENTS shared)

    list(APPEND BOREALIS_INCLUDE ${tinyxml2_INCLUDE_DIRS})
    list(APPEND BRLS_PLATFORM_LIBS tinyxml2::tinyxml2)
else()
    add_library(tinyxml2 ${BOREALIS_PATH}/lib/extern/tinyxml2/tinyxml2.cpp)
    target_include_directories(tinyxml2 PUBLIC ${BOREALIS_PATH}/include/borealis/extern/tinyxml2)
    list(APPEND BRLS_PLATFORM_LIBS tinyxml2)
endif()

# dbus
if (UNIX AND NOT APPLE AND NOT ANDROID)
    find_package(DBus)
    list(APPEND BOREALIS_INCLUDE ${DBUS_INCLUDE_DIRS})
    list(APPEND BRLS_PLATFORM_LIBS ${DBUS_LIBRARIES})
endif ()

if (PLATFORM_DESKTOP)
    if (USE_SDL2)
        if (USE_SYSTEM_SDL2)
            find_package(SDL2 REQUIRED)
        else ()
            set(BUILD_SHARED_LIBS OFF)
            set(SDL_TEST_ENABLED_BY_DEFAULT OFF)
            add_subdirectory(${BOREALIS_PATH}/lib/extern/SDL EXCLUDE_FROM_ALL)
        endif ()
        list(APPEND BRLS_PLATFORM_LIBS SDL2::SDL2-static)
        list(APPEND BRLS_PLATFORM_OPTION -D__SDL2__)
        list(APPEND BOREALIS_SOURCE ${BOREALIS_PATH}/lib/platforms/sdl)
        if (APPLE)
            list(APPEND BRLS_PLATFORM_LIBS "-framework Cocoa" "-framework IOKit")
        endif ()
    else ()
        if (USE_SYSTEM_GLFW)
            find_package(glfw3 REQUIRED)
        else ()
            set(GLFW_BUILD_DOCS OFF)
            add_subdirectory(${BOREALIS_PATH}/lib/extern/glfw EXCLUDE_FROM_ALL)
        endif ()
        list(APPEND BRLS_PLATFORM_LIBS glfw)
        list(APPEND BRLS_PLATFORM_OPTION -D__GLFW__)
        list(APPEND BOREALIS_SOURCE ${BOREALIS_PATH}/lib/platforms/glfw)
    endif ()
    if (WIN32)
        if (BOREALIS_USE_D3D11)
            list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/driver/d3d11.cpp)
            list(APPEND BRLS_PLATFORM_LIBS d3d11)
        endif ()
        if (MINGW)
            list(APPEND BRLS_PLATFORM_OPTION -DWINVER=0x0605)
        endif ()
        list(APPEND BRLS_PLATFORM_LIBS wlanapi iphlpapi ws2_32)
    elseif (APPLE)
        list(APPEND BRLS_PLATFORM_LIBS "-framework CoreWLAN" "-framework SystemConfiguration")
        list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/desktop/desktop_darwin.mm)
    endif ()
    list(APPEND BOREALIS_SOURCE ${BOREALIS_PATH}/lib/platforms/desktop)
    set(BRLS_PLATFORM_RESOURCES_PATH "\"${BRLS_RESOURCES_DIR}/resources/\"")
elseif (PLATFORM_PSV)
    if (USE_SYSTEM_SDL2)
        find_package(SDL2 REQUIRED)
    else ()
        set(BUILD_SHARED_LIBS OFF)
        set(VIDEO_VITA_PVR ON CACHE BOOL "")
        add_subdirectory(${BOREALIS_PATH}/lib/extern/SDL EXCLUDE_FROM_ALL)
    endif ()
    list(APPEND BOREALIS_SOURCE
            ${BOREALIS_PATH}/lib/platforms/psv
            ${BOREALIS_PATH}/lib/platforms/sdl
            ${BOREALIS_PATH}/lib/platforms/desktop)
    list(APPEND BRLS_PLATFORM_LIBS
            SDL2::SDL2-static
            libgpu_es4_ext_stub_weak
            libIMGEGL_stub_weak
            libGLESv2_stub_weak
            -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
            )
    list(APPEND BRLS_PLATFORM_OPTION -D__SDL2__ -D__PSV__ -DUSE_GLES2)
    set(BRLS_PLATFORM_RESOURCES_PATH "\"app0:resources/\"")
    set_target_properties(yogacore PROPERTIES POSITION_INDEPENDENT_CODE OFF)
elseif(PLATFORM_PS4)
    find_package(SDL2 REQUIRED)
    list(APPEND BOREALIS_SOURCE
            ${BOREALIS_PATH}/lib/platforms/sdl
            ${BOREALIS_PATH}/lib/platforms/desktop
            ${BOREALIS_PATH}/lib/extern/nanovg/ps4)
    file(GLOB_RECURSE PS4_PLATFORM_SRC "${BOREALIS_PATH}/lib/platforms/ps4/*.cpp")
    list(APPEND BOREALIS_SRC ${PS4_PLATFORM_SRC})
    list(APPEND BOREALIS_INCLUDE /opt/OpenOrbis-PS4-Toolchain/include)
    list(APPEND BRLS_PLATFORM_LIBS SDL2::SDL2-static SceNetCtl SceImeDialog SceCommonDialog)
    list(APPEND BRLS_PLATFORM_OPTION -D__SDL2__ -DPS4 -DUSE_GLES2)
    if (LIBJBC)
        list(APPEND BRLS_PLATFORM_LIBS jbc)
        list(APPEND BRLS_PLATFORM_OPTION -DUSE_JBC)
        set(BRLS_PLATFORM_RESOURCES_PATH "\"/mnt/sandbox/${PSN_TITLE_ID}_000/app0/resources/\"")
    else ()
        set(BRLS_PLATFORM_RESOURCES_PATH "\"/app0/resources/\"")
    endif ()
elseif(PLATFORM_ANDROID)
    find_library(SDL2 NAMES SDL2 SDL2main)
    list(APPEND BOREALIS_SOURCE
            ${BOREALIS_PATH}/lib/platforms/sdl
            ${BOREALIS_PATH}/lib/platforms/desktop
            ${BOREALIS_PATH}/lib/platforms/android)
    list(APPEND BRLS_PLATFORM_LIBS SDL2 SDL2main)
    list(APPEND BRLS_PLATFORM_OPTION -D__SDL2__ -DANDROID -DUSE_GLES2)
elseif(PLATFORM_IOS)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(${BOREALIS_PATH}/lib/extern/SDL)
    find_library(SDL2 NAMES SDL2-static SDL2main)
    list(APPEND BOREALIS_SOURCE
            ${BOREALIS_PATH}/lib/platforms/sdl
            ${BOREALIS_PATH}/lib/platforms/desktop)
    list(APPEND BRLS_PLATFORM_LIBS SDL2-static SDL2main)
    list(APPEND BRLS_PLATFORM_OPTION -D__SDL2__ -DIOS -DUSE_GLES3)
elseif(PLATFORM_SWITCH)
    list(APPEND BOREALIS_SOURCE
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/archive
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/bfgrp
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/bfsar
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/bfwar
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/bfwav
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/bfwsd
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/player
            )
    file(GLOB_RECURSE SWITCH_PLATFORM_SRC "${BOREALIS_PATH}/lib/platforms/switch/*.cpp")
    list(APPEND BOREALIS_SRC ${SWITCH_PLATFORM_SRC})
    list(APPEND BOREALIS_INCLUDE ${BOREALIS_PATH}/lib/extern/switch-libpulsar/include)
    list(APPEND BRLS_PLATFORM_OPTION $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile> -D__SWITCH__ -DSTBI_NO_THREAD_LOCALS)
    find_package(PkgConfig REQUIRED)

    if (BOREALIS_USE_DEKO3D)
        list(APPEND BRLS_PLATFORM_LIBS deko3d nx m)
    elseif (USE_SDL2)
        list(REMOVE_ITEM BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/switch/switch_video.cpp)
        list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/sdl/sdl_video.cpp)
        list(APPEND BRLS_PLATFORM_LIBS sdl2 EGL glapi drm_nouveau nx m)
    else ()
        list(REMOVE_ITEM BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/switch/switch_video.cpp)
        list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/glfw/glfw_video.cpp)
        list(APPEND BRLS_PLATFORM_LIBS glfw3 EGL glapi drm_nouveau nx m)
    endif ()
    set(BRLS_PLATFORM_RESOURCES_PATH "\"romfs:/\"")
else ()
    message(FATAL_ERROR "Please set build target. Example: -DPLATFORM_DESKTOP=ON or -DPLATFORM_SWITCH=ON")
endif ()

# borealis c/cpp source
list(APPEND BOREALIS_INCLUDE
        ${BOREALIS_PATH}/include
        ${BOREALIS_PATH}/include/borealis/extern
        ${BOREALIS_PATH}/include/borealis/extern/nanovg
        )
list(APPEND BOREALIS_SOURCE
        ${BOREALIS_PATH}/lib/core
        ${BOREALIS_PATH}/lib/core/touch
        ${BOREALIS_PATH}/lib/views
        ${BOREALIS_PATH}/lib/views/cells
        ${BOREALIS_PATH}/lib/views/widgets
        ${BOREALIS_PATH}/lib/extern/glad
        ${BOREALIS_PATH}/lib/extern/libretro-common/compat
        ${BOREALIS_PATH}/lib/extern/libretro-common/encodings
        ${BOREALIS_PATH}/lib/extern/libretro-common/features
        )
if (PLATFORM_PSV OR PLATFORM_PS4)
        list(REMOVE_ITEM BOREALIS_SOURCE ${BOREALIS_PATH}/lib/extern/glad)
endif ()

# nanovg
list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/extern/nanovg/nanovg.c)
if (PLATFORM_SWITCH AND BOREALIS_USE_DEKO3D)
    file(GLOB_RECURSE NANOVG_PLATFORM_SRC ${BOREALIS_PATH}/lib/extern/nanovg/*.cpp)
    list(APPEND BOREALIS_SRC ${NANOVG_PLATFORM_SRC})
endif ()
message(STATUS "BOREALIS_SOURCE: ${BOREALIS_SOURCE}")

foreach (lib ${BOREALIS_SOURCE})
    file(GLOB_RECURSE TEMP_SRC "${lib}/*.cpp" "${lib}/*.c")
    list(APPEND BOREALIS_SRC ${TEMP_SRC})
endforeach (lib)

# borealis 图形 driver
set(BOREALIS_DRIVER BOREALIS_USE_OPENGL)
if (BOREALIS_USE_METAL)
    set(BOREALIS_DRIVER BOREALIS_USE_METAL)
elseif (BOREALIS_USE_D3D11)
    set(BOREALIS_DRIVER BOREALIS_USE_D3D11)
elseif (BOREALIS_USE_DEKO3D)
    set(BOREALIS_DRIVER BOREALIS_USE_DEKO3D)
endif ()
list(APPEND BRLS_PLATFORM_OPTION -D${BOREALIS_DRIVER})
message(STATUS "borealis driver ${BOREALIS_DRIVER}")

# borealis Library
add_library(borealis STATIC ${BOREALIS_SRC})
set_property(TARGET borealis PROPERTY CXX_STANDARD 17)
target_include_directories(borealis PUBLIC ${BOREALIS_INCLUDE})
target_link_libraries(borealis fmt tweeny yogacore ${BRLS_PLATFORM_LIBS})
target_compile_options(borealis PUBLIC
        -DBRLS_RESOURCES=${BRLS_PLATFORM_RESOURCES_PATH}
        -DYG_ENABLE_EVENTS
        -ffunction-sections
        -fdata-sections
        ${BRLS_PLATFORM_OPTION}
        )
