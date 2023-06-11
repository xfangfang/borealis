cmake_minimum_required(VERSION 3.15)

# build options
option(PLATFORM_DESKTOP "build for linux macos or windows" OFF)
option(PLATFORM_SWITCH "build for NintendoSwitch" OFF)
option(PLATFORM_PSV "build for PSVita" OFF)
option(USE_GLFW "using glfw for input and create window" ON)
option(USE_SDL2 "using sdl2 for input and create window" OFF)
option(USE_SHARED_LIB "using shared library" OFF)

if(USE_SDL2)
    message("SDL2")
    set(USE_GLFW OFF)
elseif(USE_GLFW)
    message("GLFW")
    set(USE_SDL2 OFF)
endif()


# toolchain
if (PLATFORM_DESKTOP)
    message("building for Desktop")
    set(TARGET_PLATFORM desktop CACHE STRING "")
    message("Build Type: ${CMAKE_BUILD_TYPE}")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb -Wall")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
    if (USE_SHARED_LIB)
        message("Using shared library")
        SET(_USE_SHARED_LIB ON)
    endif ()
elseif (PLATFORM_SWITCH)
    message("building for SWITCH")
    message("Build Type: ${CMAKE_BUILD_TYPE}")
    set(CMAKE_USE_SYSTEM_ENVIRONMENT_PATH OFF)
    file(TO_CMAKE_PATH "$ENV{DEVKITPRO}" DEVKITPRO)
    if (NOT IS_DIRECTORY ${DEVKITPRO})
        message(FATAL_ERROR "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
    endif ()
    set(DEVKITPRO $ENV{DEVKITPRO} CACHE BOOL "DEVKITPRO")
    include(${DEVKITPRO}/cmake/Switch.cmake REQUIRED)
    set(__SWITCH__ ON)
    set(CMAKE_C_FLAGS "-I${DEVKITPRO}/libnx/include -I${DEVKITPRO}/portlibs/switch/include")
    set(CMAKE_CXX_FLAGS "-I${DEVKITPRO}/libnx/include -I${DEVKITPRO}/portlibs/switch/include")
elseif(PLATFORM_PSV)
    message("building for PSVita")
    set(USE_GLFW ON)
    set(USE_SDL2 OFF)
    message("Force using GLFW")
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
      if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
      else()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
      endif()
    endif()
    set(VITASDK $ENV{VITASDK} CACHE BOOL "VITASDK")
    include("${VITASDK}/share/vita.cmake" REQUIRED)
else()
    message(FATAL_ERROR "Please set build target -DPLATFORM_DESKTOP=ON or -DPLATFORM_SWITCH=ON or -DPLATFORM_PSV=ON")
endif ()

# project info
project(borealis_demo)
set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
set(VERSION_REVISION "0")
set(PROJECT_TITLEID "0x010FF000FFFF00FF")
set(VITA_TITLEID  "BOREALIS0")
set(VITA_VERSION  "01.00")
set(PROJECT_AUTHOR "borealis")
set(PROJECT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/img/demo_icon.jpg)
set(PROJECT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources)


add_subdirectory(library)


# setting src and include
file(GLOB_RECURSE MAIN_SRC demo/*.cpp)

set(PLATFORM_OPTION)
if (PLATFORM_SWITCH)
    set(PLATFORM_LIBS
            # needed by borealis
            glfw3 EGL glapi drm_nouveau
            # base lib
            nx m
            )
    list(APPEND MAIN_SRC ${CMAKE_SOURCE_DIR}/library/lib/platforms/switch/switch_wrapper.c)
elseif(PLATFORM_PSV)
    set(PLATFORM_LIBS
        glfw3
        libgpu_es4_ext_stub_weak
        libGLESv2_stub_weak
        libIMGEGL_stub_weak
        SceCtrl_stub
        SceTouch_stub
        pthread
    )
    add_definitions(-D__psp2__ -D__PSV__)
endif()


# building target
add_executable(${PROJECT_NAME} ${MAIN_SRC})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
target_include_directories(${PROJECT_NAME} PRIVATE demo)
target_compile_options(${PROJECT_NAME} PRIVATE
        -ffunction-sections
        -fdata-sections
        ${PLATFORM_OPTION}
        )
target_link_libraries(${PROJECT_NAME} PRIVATE
        borealis
        ${PLATFORM_LIBS}
        )


# building release file
if (PLATFORM_DESKTOP)
    add_custom_target(${PROJECT_NAME}.data
            COMMAND "${CMAKE_COMMAND}" -E copy_directory ${CMAKE_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources
            )
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}.data)
elseif(PLATFORM_PSV)
    vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
    vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
      VERSION ${VITA_VERSION}
      NAME ${PROJECT_NAME}
      FILE ${CMAKE_SOURCE_DIR}/resources resources
      FILE ${CMAKE_SOURCE_DIR}/psv/sce_sys sce_sys
      FILE ${CMAKE_SOURCE_DIR}/psv/module/ module
    )
else ()
    add_custom_target(${PROJECT_NAME}.nro
            DEPENDS ${PROJECT_NAME}
            COMMAND ${NX_NACPTOOL_EXE} --create "${PROJECT_NAME}"
            "${PROJECT_AUTHOR}" "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}"
            ${PROJECT_NAME}.nacp --titleid=${PROJECT_TITLEID}
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_BINARY_DIR}/resources
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/resources/font
            COMMAND ${NX_ELF2NRO_EXE} ${PROJECT_NAME}.elf ${PROJECT_NAME}.nro --icon=${PROJECT_ICON}
            --nacp=${PROJECT_NAME}.nacp --romfsdir=${CMAKE_BINARY_DIR}/resources
            )
endif ()
