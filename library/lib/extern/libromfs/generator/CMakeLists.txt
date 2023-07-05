cmake_minimum_required(VERSION 3.16)
project(libromfs-generator)
set(CMAKE_CXX_STANDARD 20)

# Add sources
add_executable(${PROJECT_NAME}
    source/main.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE include)