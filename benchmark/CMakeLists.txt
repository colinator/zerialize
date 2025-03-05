cmake_minimum_required(VERSION 3.15)

project(benchmark_zerialize)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(.. ${CMAKE_BINARY_DIR}/zerialize)
add_executable(${PROJECT_NAME} benchmark_zerialize.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE zerialize)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
