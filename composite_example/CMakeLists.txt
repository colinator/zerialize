cmake_minimum_required(VERSION 3.10)

project(test_zerialize_composite VERSION 1.0)

include(FetchContent)

FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY https://github.com/google/flatbuffers.git
    GIT_TAG master
)
FetchContent_MakeAvailable(flatbuffers)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test_zerialize_composite test_zerialize_composite.cpp)

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${flatbuffers_SOURCE_DIR}/include
    nlohmann_json::nlohmann_json
)

# Link against flatbuffers
target_link_libraries(${PROJECT_NAME} PRIVATE 
    nlohmann_json::nlohmann_json 
    flatbuffers
)

# Add compiler warnings
if(MSVC)
    target_compile_options(test_zerialize_composite PRIVATE /W4)
else()
    target_compile_options(test_zerialize_composite PRIVATE -Wall -Wextra)
endif()
