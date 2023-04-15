cmake_minimum_required(VERSION 3.15)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
set(BOREALIS_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# using custom glfw
if ((NOT USE_SDL2) AND PLATFORM_DESKTOP)
    set(GLFW_BUILD_DOCS OFF)
    add_subdirectory(${BOREALIS_PATH}/lib/extern/glfw EXCLUDE_FROM_ALL)
endif ()

# fmt
add_subdirectory(${BOREALIS_PATH}/lib/extern/fmt EXCLUDE_FROM_ALL)

set(BOREALIS_INCLUDE
        ${BOREALIS_PATH}/include
        ${BOREALIS_PATH}/include/borealis/extern
        ${BOREALIS_PATH}/include/borealis/extern/tinyxml2
        ${BOREALIS_PATH}/lib/extern/yoga/src
        ${BOREALIS_PATH}/lib/extern/tweeny/include
        ${BOREALIS_PATH}/include/borealis/extern/nanovg-gl
        )
set(BOREALIS_SOURCE
        ${BOREALIS_PATH}/lib/core
        ${BOREALIS_PATH}/lib/core/touch
        ${BOREALIS_PATH}/lib/views
        ${BOREALIS_PATH}/lib/views/cells
        ${BOREALIS_PATH}/lib/views/widgets
        ${BOREALIS_PATH}/lib/extern/glad
        ${BOREALIS_PATH}/lib/extern/tinyxml2
        ${BOREALIS_PATH}/lib/extern/libretro-common/compat
        ${BOREALIS_PATH}/lib/extern/libretro-common/encodings
        ${BOREALIS_PATH}/lib/extern/libretro-common/features
        ${BOREALIS_PATH}/lib/extern/yoga/src/yoga
        ${BOREALIS_PATH}/lib/extern/yoga/src/yoga/event
        ${BOREALIS_PATH}/lib/extern/nanovg-gl
        )

if (NOT BRLS_RESOURCES_DIR)
    set(BRLS_RESOURCES_DIR ".")
endif ()

set(BOREALIS_SRC)
if (PLATFORM_DESKTOP)
    if (USE_SDL2)
        message("using SDL2")
        find_package(SDL2 REQUIRED)
        message("=====> ${SDL2_INCLUDE_DIR}, ${SDL2_LIBRARIES}")
        list(APPEND BOREALIS_SOURCE
                ${BOREALIS_PATH}/lib/platforms/sdl
                ${BOREALIS_PATH}/lib/platforms/desktop)
        set(BRLS_PLATFORM_LIBS ${SDL2_LIBRARIES})
        set(BRLS_PLATFORM_OPTION -D__SDL2__ -I/usr/local/include -I${SDL2_INCLUDE_DIR})
    else ()
        message("using GLFW3")
        list(APPEND BOREALIS_SOURCE
                ${BOREALIS_PATH}/lib/platforms/glfw
                ${BOREALIS_PATH}/lib/platforms/desktop)
        set(BRLS_PLATFORM_LIBS glfw)
        set(BRLS_PLATFORM_OPTION -D__GLFW__)
    endif ()
    set(BRLS_PLATFORM_RESOURCES_PATH "\"${BRLS_RESOURCES_DIR}/resources/\"")
elseif (PLATFORM_PSV)
    list(APPEND BOREALIS_SOURCE
            ${BOREALIS_PATH}/lib/platforms/psv
            ${BOREALIS_PATH}/lib/platforms/glfw
            ${BOREALIS_PATH}/lib/platforms/desktop)
    list(REMOVE_ITEM BOREALIS_SOURCE ${BOREALIS_PATH}/lib/extern/glad)
    set(BRLS_PLATFORM_LIBS)
    set(BRLS_PLATFORM_OPTION -D__GLFW__ -D__PSV__)
    set(BRLS_PLATFORM_RESOURCES_PATH "\"app0:resources/\"")
else ()
    list(APPEND BOREALIS_SOURCE
            ${BOREALIS_PATH}/lib/platforms/switch
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/archive
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/bfgrp
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/bfsar
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/bfwar
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/bfwav
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/bfwsd
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/src/player
            )
    list(APPEND BOREALIS_INCLUDE
            ${BOREALIS_PATH}/lib/extern/switch-libpulsar/include
            )
    set(BRLS_PLATFORM_OPTION $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile> -D__SWITCH__ -DSTBI_NO_THREAD_LOCALS)
    if (USE_SDL2)
        list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/sdl/sdl_video.cpp)
        set(BRLS_PLATFORM_LIBS sdl2 EGL glapi drm_nouveau nx m)
        list(APPEND BRLS_PLATFORM_OPTION -D__SDL2__)
    else ()
        list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/glfw/glfw_video.cpp)
        set(BRLS_PLATFORM_LIBS glfw3 EGL glapi drm_nouveau nx m)
        list(APPEND BRLS_PLATFORM_OPTION -D__GLFW__)
    endif ()
    set(BRLS_PLATFORM_RESOURCES_PATH "\"romfs:/\"")
endif ()
foreach (lib ${BOREALIS_SOURCE})
    aux_source_directory(${lib} TEMP_SRC)
    list(APPEND BOREALIS_SRC ${TEMP_SRC})
endforeach (lib)

if (APPLE)
elseif (UNIX)
    find_package(DBus)
    list(APPEND BOREALIS_INCLUDE ${DBUS_INCLUDE_DIRS})
    list(APPEND BRLS_PLATFORM_LIBS ${DBUS_LIBRARIES})
endif ()

add_library(borealis STATIC ${BOREALIS_SRC})
set_property(TARGET borealis PROPERTY CXX_STANDARD 17)
target_include_directories(borealis PUBLIC ${BOREALIS_INCLUDE})
target_link_libraries(borealis fmt ${BRLS_PLATFORM_LIBS})
target_compile_options(borealis PUBLIC
        -DBRLS_RESOURCES=${BRLS_PLATFORM_RESOURCES_PATH}
        -DYG_ENABLE_EVENTS
        -ffunction-sections
        -fdata-sections
        ${BRLS_PLATFORM_OPTION}
        )

if (BRLS_USE_OPENCC)
    target_include_directories(borealis PUBLIC ${BRLS_OPENCC_INCLUDE})
    target_link_libraries(borealis libopencc)
    target_compile_options(borealis PUBLIC -DOPENCC)
endif ()