cmake_minimum_required(VERSION 3.10)

# build options
option(PLATFORM_DESKTOP "build for linux macos or windows" OFF)
option(PLATFORM_SWITCH "build for NintendoSwitch" OFF)
option(PLATFORM_PSV "build for PSVita" OFF)
option(PLATFORM_ANDROID "build for android" OFF)
option(PLATFORM_IOS "build for ios" OFF)
option(USE_GLFW "using glfw for input and create window" ON)
option(USE_SDL2 "using sdl2 for input and create window" OFF)
option(USE_SHARED_LIB "using shared library" OFF)
option(USE_LIBROMFS "using libromfs to bundle resources" OFF)

if (APPLE AND NOT PLATFORM_IOS)
    option(BUNDLE_MACOS_APP "Bundle a app for macOS" OFF)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "" FORCE)
    set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET[arch=arm64] "11.0" CACHE STRING "" FORCE)
    message(STATUS "CMAKE_OSX_ARCHITECTURES: ${CMAKE_OSX_ARCHITECTURES}")
endif ()

# toolchain
if (PLATFORM_DESKTOP)
    message(STATUS "building for Desktop")
    set(TARGET_PLATFORM desktop CACHE STRING "")
    message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb -Wall")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
    if (USE_SHARED_LIB)
        message(STATUS "Using shared library")
        SET(_USE_SHARED_LIB ON)
    endif ()
elseif (PLATFORM_SWITCH)
    message(STATUS "building for SWITCH")
    message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
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
    message(STATUS "building for PSVita")
    set(USE_GLFW ON)
    set(USE_SDL2 OFF)
    message(STATUS "Force using GLFW")
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
      if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
      else()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
      endif()
    endif()
    set(VITASDK $ENV{VITASDK} CACHE BOOL "VITASDK")
    include("${VITASDK}/share/vita.cmake" REQUIRED)
elseif(PLATFORM_ANDROID)
    message(STATUS "building for Android")
    set(USE_GLFW OFF)
    set(USE_SDL2 ON)
    SET(USE_LIBROMFS ON)
elseif (PLATFORM_IOS)
    message(STATUS "building for iOS series")
    set(USE_GLFW OFF)
    set(USE_SDL2 ON)
    SET(USE_LIBROMFS ON)
    SET(LIBROMFS_PREBUILT_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/libromfs-generator" CACHE STRING "")
    if (NOT EXISTS "${LIBROMFS_PREBUILT_GENERATOR}")
        message(FATAL_ERROR "libromfs-generator has not been built, please refer to borealis/build_libromfs_generator.sh for more information")
    endif()
else()
    message(FATAL_ERROR "Please set build target -DPLATFORM_DESKTOP=ON or -DPLATFORM_SWITCH=ON or -DPLATFORM_PSV=ON")
endif ()

message(STATUS "USE SDL2: ${USE_SDL2}")
message(STATUS "USE_GLFW: ${USE_GLFW}")

# project info
project(borealis_demo)
set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
set(VERSION_ALTER "0")
set(PROJECT_TITLEID "0x010FF000FFFF00FF")
set(VITA_TITLEID  "BOREALIS0")
set(VITA_VERSION  "01.00")
set(PROJECT_AUTHOR "borealis")
set(PROJECT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/img/demo_icon.jpg)
set(PROJECT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources)

if (USE_LIBROMFS)
    message(STATUS "USE LIBROMFS")
    set(USE_LIBROMFS ON)
    add_definitions(-DUSE_LIBROMFS)
    set(LIBROMFS_PROJECT_NAME ${PROJECT_NAME})
    set(LIBROMFS_RESOURCE_LOCATION "${PROJECT_RESOURCES}")
endif ()

add_subdirectory(library)


# setting src and include
file(GLOB_RECURSE MAIN_SRC demo/*.cpp)

set(PLATFORM_OPTION)
if (PLATFORM_DESKTOP)
    if (WIN32)
        list(APPEND MAIN_SRC ${CMAKE_SOURCE_DIR}/demo/resource.rc)
    elseif (BUNDLE_MACOS_APP)
        list(APPEND MAIN_SRC ${CMAKE_SOURCE_DIR}/demo/borealis.icns)
    endif ()
elseif (PLATFORM_SWITCH)
    set(PLATFORM_LIBS
        # needed by borealis
        glfw3 EGL glapi drm_nouveau
        # base lib
        nx m
    )
    list(APPEND MAIN_SRC ${CMAKE_SOURCE_DIR}/library/lib/platforms/switch/switch_wrapper.c)
elseif (PLATFORM_PSV)
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
endif ()


# building target
if (PLATFORM_ANDROID)
    add_library(${PROJECT_NAME} SHARED)
    target_sources(${PROJECT_NAME} PRIVATE ${MAIN_SRC})
else()
    add_executable(${PROJECT_NAME} ${MAIN_SRC})
endif ()
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
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
    if (BUNDLE_MACOS_APP)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
            MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}"
            MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}"
            MACOSX_BUNDLE_ICON_FILE "borealis.icns"
            MACOSX_BUNDLE_COPYRIGHT "Copyright 2023 ${PROJECT_AUTHOR}"
            RESOURCE "${CMAKE_SOURCE_DIR}/demo/borealis.icns"
        )
        add_custom_target(${PROJECT_NAME}.data
                COMMAND "${CMAKE_COMMAND}" -E copy_directory ${PROJECT_RESOURCES} ${PROJECT_NAME}.app/Contents/Resources/resources
        )
    else ()
        add_custom_target(${PROJECT_NAME}.data
                COMMAND "${CMAKE_COMMAND}" -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources
        )
    endif ()
    if (NOT USE_LIBROMFS)
        add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}.data)
    endif ()
elseif (PLATFORM_PSV)
    vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
    vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
        VERSION ${VITA_VERSION}
        NAME ${PROJECT_NAME}
        FILE ${CMAKE_SOURCE_DIR}/resources resources
        FILE ${CMAKE_SOURCE_DIR}/psv/sce_sys sce_sys
        FILE ${CMAKE_SOURCE_DIR}/psv/module/ module
    )
elseif (PLATFORM_SWITCH)
    add_custom_target(${PROJECT_NAME}.nro DEPENDS ${PROJECT_NAME}
        COMMAND ${NX_NACPTOOL_EXE} --create "${PROJECT_NAME}" "${PROJECT_AUTHOR}" "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}" ${PROJECT_NAME}.nacp --titleid=${PROJECT_TITLEID}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_BINARY_DIR}/resources
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/resources/font
        COMMAND ${NX_ELF2NRO_EXE} ${PROJECT_NAME}.elf ${PROJECT_NAME}.nro --icon=${PROJECT_ICON} --nacp=${PROJECT_NAME}.nacp --romfsdir=${CMAKE_BINARY_DIR}/resources
    )
elseif (PLATFORM_IOS)
    set(IOS_CODE_SIGN_IDENTITY "" CACHE STRING "The code sign identity to use when building the IPA.")
    if(IOS_CODE_SIGN_IDENTITY STREQUAL "")
        set(IOS_CODE_SIGNING_ENABLED NO)
    else()
        set(IOS_CODE_SIGNING_ENABLED YES)
    endif()
    if (NOT DEFINED IOS_GUI_IDENTIFIER)
        set(IOS_GUI_IDENTIFIER "com.borealis.demo")
    endif ()
    if (TVOS)
        set(IOS_SPLASH_STORYBOARD "${CMAKE_SOURCE_DIR}/demo/ios/tvos/Splash.storyboard")
    else ()
        set(IOS_SPLASH_STORYBOARD "${CMAKE_SOURCE_DIR}/demo/ios/iphoneos/Splash.storyboard")
    endif ()
    set(IOS_ASSETS ${CMAKE_SOURCE_DIR}/demo/ios/Images.xcassets)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${IOS_CODE_SIGN_IDENTITY}"
            XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "${IOS_CODE_SIGNING_ENABLED}"
            XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "${IOS_CODE_SIGNING_ENABLED}"
            BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/demo/ios/iOSBundleInfo.plist.in
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER ${IOS_GUI_IDENTIFIER}
            MACOSX_BUNDLE_BUNDLE_NAME Borealis
            MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}"
            XCODE_ATTRIBUTE_ENABLE_BITCODE NO
            XCODE_ATTRIBUTE_SKIP_INSTALL NO
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
            )
    set_property(
            SOURCE ${IOS_ASSETS}
            PROPERTY MACOSX_PACKAGE_LOCATION "Resources"
    )
    set_property(
            SOURCE ${IOS_SPLASH_STORYBOARD}
            PROPERTY MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${PROJECT_NAME} PRIVATE
            ${IOS_ASSETS}
            ${IOS_SPLASH_STORYBOARD}
    )
endif ()
