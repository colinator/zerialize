cmake_minimum_required(VERSION 3.15)

project(benchmark_compare)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

#------------------------------------------------------------------------------
# Configure reflect-cpp options
#------------------------------------------------------------------------------
set(REFLECTCPP_FLEXBUFFERS ON CACHE BOOL "Enable flexbuffers support for reflect-cpp" FORCE)
set(REFLECTCPP_FLATBUFFERS ON CACHE BOOL "Enable flatbuffers support for reflect-cpp" FORCE)
#set(REFLECTCPP_MSGPACK ON CACHE BOOL "Enable message pack support for reflect-cpp" FORCE)
set(REFLECTCPP_JSON        ON CACHE BOOL "Enable JSON support" FORCE)

#------------------------------------------------------------------------------
# Fetch and configure flatbuffers
#------------------------------------------------------------------------------
# Disable building executables and tests to avoid duplicate target definitions.
set(FLATBUFFERS_BUILD_FLATC OFF CACHE BOOL "Disable flatc" FORCE)
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "Disable flatbuffers tests" FORCE)
FetchContent_Declare(
  flatbuffers
  GIT_REPOSITORY https://github.com/google/flatbuffers.git
  GIT_TAG master  # or a specific commit/tag as needed
)
FetchContent_MakeAvailable(flatbuffers)

# flatbuffers might create a target named "flatbuffers" but reflect-cpp expects the alias
if (NOT TARGET flatbuffers::flatbuffers)
  add_library(flatbuffers::flatbuffers ALIAS flatbuffers)
endif()

#------------------------------------------------------------------------------
# Fetch and configure messagepack
#------------------------------------------------------------------------------
# FetchContent_Declare(
#     msgpack
#     GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
#     GIT_TAG cpp_master
# )
# FetchContent_GetProperties(msgpack)
# if(NOT msgpack_POPULATED)
#     FetchContent_Populate(msgpack)
#     # Explicitly disable Boost
#     set(MSGPACK_BOOST OFF CACHE BOOL "Disable Boost dependency" FORCE)
#     set(MSGPACK_USE_BOOST OFF CACHE BOOL "Disable Boost dependency" FORCE)
#     set(MSGPACK_BUILD_EXAMPLES OFF CACHE BOOL "Don't build examples" FORCE)
#     set(MSGPACK_BUILD_TESTS OFF CACHE BOOL "Don't build tests" FORCE)
#     add_subdirectory(${msgpack_SOURCE_DIR} ${msgpack_BINARY_DIR})
# endif()

# if (NOT TARGET msgpack::msgpack)
#   add_library(msgpack::msgpack ALIAS msgpack)
# endif()

#------------------------------------------------------------------------------
# Fetch reflect-cpp
#------------------------------------------------------------------------------
FetchContent_Declare(
  reflect-cpp
  GIT_REPOSITORY https://github.com/getml/reflect-cpp.git
  GIT_TAG main
)
FetchContent_MakeAvailable(reflect-cpp)

#------------------------------------------------------------------------------
# Just include zerialize
#------------------------------------------------------------------------------
add_subdirectory(.. ${CMAKE_BINARY_DIR}/zerialize)

#------------------------------------------------------------------------------
# Build your executable
#------------------------------------------------------------------------------
add_executable(${PROJECT_NAME} benchmark_compare.cpp reflect_cpp_impl.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${reflect-cpp_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE reflectcpp zerialize)



# cmake_minimum_required(VERSION 3.15)

# project(benchmark_compare)

# set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # # Add reflect-cpp
# # #set(REFLECTCPP_FLEXBUFFERS ON CACHE BOOL "Enable flexbuffers support" FORCE)
# # # set(REFLECTCPP_FLEXBUFFERS ON)

# add_subdirectory(third_party/flatbuffers)
# set(flatbuffers_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/flatbuffers/CMake")
# set(REFLECTCPP_FLEXBUFFERS ON CACHE BOOL "Enable flexbuffers support" FORCE)
# add_subdirectory(third_party/reflect-cpp)


# add_subdirectory(.. ${CMAKE_BINARY_DIR}/zerialize)

# # # FetchContent_MakeAvailable(flatbuffers)


# add_executable(${PROJECT_NAME} benchmark_compare.cpp) # reflect_cpp_impl.cpp src/reflectcpp_xml.cpp)

# FetchContent_Declare(
#     flatbuffers
#     GIT_REPOSITORY https://github.com/google/flatbuffers.git
#     GIT_TAG master
# )
# FetchContent_MakeAvailable(flatbuffers)

# # Link against the reflect-cpp target (the target name is as provided by its CMake config)
# target_link_libraries(${PROJECT_NAME} PRIVATE
#     zerialize
#     reflectcpp
#     flatbuffers
# )





# # include(FetchContent)
# # FetchContent_Declare(
# #     reflect-cpp
# #     GIT_REPOSITORY https://github.com/getml/reflect-cpp.git
# #     GIT_TAG main
# # )
# # FetchContent_MakeAvailable(reflect-cpp)
# # if(NOT reflect-cpp_POPULATED)
# #     FetchContent_Populate(reflect-cpp)
# #     add_subdirectory(reflect-cpp)

# #     set(REFLECTCPP_FLEXBUFFERS ON CACHE BOOL "Enable flexbuffers support" FORCE)
# #     #add_subdirectory(${reflect-cpp_SOURCE_DIR} ${reflect-cpp_BINARY_DIR})
# # endif()

# # # Add zerialize
# # add_subdirectory(.. ${CMAKE_BINARY_DIR}/zerialize)

# # # # Add executable
# # add_executable(${PROJECT_NAME} benchmark_compare.cpp)

# # # Include the reflect-cpp header-only library
# # target_include_directories(${PROJECT_NAME} PRIVATE 
# #     ${reflect-cpp_SOURCE_DIR}/include
# # )

# # # Link libraries
# # target_link_libraries(${PROJECT_NAME} PRIVATE 
# #     zerialize
# #     reflectcpp
# #     #reflect-cpp
# # )

# # # Compiler warnings
# # # if(MSVC)
# # #     target_compile_options(${PROJECT_NAME} PRIVATE /W4)
# # # else()
# # #     target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
# # # endif()
