cmake_minimum_required(VERSION 3.15)

project(examples)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(.. ${CMAKE_BINARY_DIR}/zerialize)

add_executable(basic_usage general_usage.cpp)
target_link_libraries(basic_usage PRIVATE zerialize)

add_executable(general_usage general_usage.cpp)
target_link_libraries(general_usage PRIVATE zerialize)

if(MSVC)
    target_compile_options(basic_usage PRIVATE /W4)
    target_compile_options(general_usage PRIVATE /W4)
else()
    target_compile_options(basic_usage PRIVATE -Wall -Wextra)
    target_compile_options(general_usage PRIVATE -Wall -Wextra)
endif()
