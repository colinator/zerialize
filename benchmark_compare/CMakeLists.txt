cmake_minimum_required(VERSION 3.15)

project(benchmark_compare LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
# set(CMAKE_CXX_FLAGS "-O3 -DNDEBUG")

find_package(reflectcpp CONFIG REQUIRED)

add_subdirectory(.. ${CMAKE_BINARY_DIR}/zerialize)

add_executable(${PROJECT_NAME} 
    src/benchmark_compare.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    reflectcpp::reflectcpp
    zerialize
)