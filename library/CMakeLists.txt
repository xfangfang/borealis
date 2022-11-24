cmake_minimum_required(VERSION 3.15)


set(BOREALIS_PATH ${CMAKE_CURRENT_SOURCE_DIR})

set(BOREALIS_INCLUDE
        ${BOREALIS_PATH}/include
        ${BOREALIS_PATH}/lib/extern/fmt/include
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

set(BOREALIS_SRC)
if (PLATFORM_DESKTOP)
    find_package(glfw3 REQUIRED)
    find_package(glm REQUIRED)
    list(APPEND BOREALIS_SOURCE
            ${BOREALIS_PATH}/lib/platforms/glfw
            ${BOREALIS_PATH}/lib/extern/fmt/src/
            )
    list(APPEND BOREALIS_SRC
            ${BOREALIS_PATH}/lib/platforms/switch/swkbd.cpp
            )
    set(BRLS_PLATFORM_RESOURCES_PATH "\"./resources/\"")
    set(BRLS_PLATFORM_LIBS glfw)
    set(BRLS_PLATFORM_OPTION -DNO_TOUCH_SCROLLING=true -D__GLFW__)
else ()
    list(APPEND BOREALIS_SOURCE
            ${BOREALIS_PATH}/lib/platforms/switch
            ${BOREALIS_PATH}/lib/extern/nxfmtwrapper
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
    list(APPEND BOREALIS_SRC ${BOREALIS_PATH}/lib/platforms/glfw/glfw_video.cpp)
    set(BRLS_PLATFORM_RESOURCES_PATH "\"romfs:/\"")
    set(BRLS_PLATFORM_LIBS glfw3 EGL glapi drm_nouveau nx m)
    set(BRLS_PLATFORM_OPTION $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile> -D__SWITCH__)
endif ()
foreach (lib ${BOREALIS_SOURCE})
    aux_source_directory(${lib} TEMP_SRC)
    list(APPEND BOREALIS_SRC ${TEMP_SRC})
endforeach (lib)


add_library(borealis STATIC ${BOREALIS_SRC})
set_property(TARGET borealis PROPERTY CXX_STANDARD 17)
target_include_directories(borealis PUBLIC ${BOREALIS_INCLUDE})
target_link_libraries(borealis ${BRLS_PLATFORM_LIBS})
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