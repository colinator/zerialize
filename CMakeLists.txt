cmake_minimum_required(VERSION 3.10)

# Project name
project(test_zerialize VERSION 1.0)

# Include FetchContent
include(FetchContent)

# Fetch flatbuffers
FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY https://github.com/google/flatbuffers.git
    GIT_TAG master
)
FetchContent_MakeAvailable(flatbuffers)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
file(GLOB SOURCES "src/*.cpp")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${flatbuffers_SOURCE_DIR}/include
)

# Link against flatbuffers
target_link_libraries(${PROJECT_NAME} PRIVATE flatbuffers)

# Optional: Add compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
