cmake_minimum_required(VERSION 3.15)

project(zerialize VERSION 1.0 LANGUAGES CXX)

include(FetchContent)

# -------------------
# Options to selectively enable protocols and math backends
option(ZERIALIZE_ENABLE_FLEXBUFFERS "Enable FlexBuffers (via FlatBuffers)" ON)
option(ZERIALIZE_ENABLE_JSON        "Enable JSON (via yyjson)" ON)
option(ZERIALIZE_ENABLE_CBOR        "Enable CBOR (via jsoncons)" ON)
option(ZERIALIZE_ENABLE_MSGPACK     "Enable MessagePack (via msgpack-c)" ON)

option(ZERIALIZE_ENABLE_XTENSOR     "Enable xtensor backend" ON)
option(ZERIALIZE_ENABLE_EIGEN       "Enable Eigen backend" ON)

option(ZERIALIZE_ENABLE_MODULES     "Build C++ modules support" OFF)

# -------------------
# Create header-only 'interface' library
add_library(${PROJECT_NAME} INTERFACE)

# Specify the C++ standard
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

# Public headers
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# -------------------
# Protocol: FlatBuffers/FlexBuffers
if(ZERIALIZE_ENABLE_FLEXBUFFERS)
  # FlexBuffers is header-only; we only need the headers on the include path.
  # If a flatbuffers target exists, reuse its include dirs; otherwise FetchContent the source
  # and add the include directory directly (no linking/building).
  set(_zerialize_flatbuffers_includes "")
  if(TARGET flatbuffers)
    get_target_property(_zerialize_flatbuffers_includes flatbuffers INTERFACE_INCLUDE_DIRECTORIES)
  elseif(TARGET flatbuffers::flatbuffers)
    get_target_property(_zerialize_flatbuffers_includes flatbuffers::flatbuffers INTERFACE_INCLUDE_DIRECTORIES)
  endif()

  if(NOT _zerialize_flatbuffers_includes)
    FetchContent_Declare(
        flatbuffers
        GIT_REPOSITORY https://github.com/google/flatbuffers.git
        GIT_TAG master
    )
    FetchContent_GetProperties(flatbuffers)
    if(NOT flatbuffers_POPULATED)
      FetchContent_Populate(flatbuffers)
    endif()
    set(_zerialize_flatbuffers_includes ${flatbuffers_SOURCE_DIR}/include)
  endif()

  target_include_directories(${PROJECT_NAME} INTERFACE ${_zerialize_flatbuffers_includes})
  target_compile_definitions(${PROJECT_NAME} INTERFACE ZERIALIZE_HAS_FLEXBUFFERS=1)
endif()

# Protocol: yyjson (JSON)
if(ZERIALIZE_ENABLE_JSON)
  set(_zerialize_yyjson_target "")
  if(TARGET yyjson)
    set(_zerialize_yyjson_target yyjson)
  elseif(TARGET yyjson::yyjson)
    set(_zerialize_yyjson_target yyjson::yyjson)
  endif()
  if(NOT _zerialize_yyjson_target)
    FetchContent_Declare(
        yyjson
        GIT_REPOSITORY https://github.com/ibireme/yyjson.git
        GIT_TAG 0.12.0
    )
    FetchContent_MakeAvailable(yyjson)
    if(TARGET yyjson)
      set(_zerialize_yyjson_target yyjson)
    elseif(TARGET yyjson::yyjson)
      set(_zerialize_yyjson_target yyjson::yyjson)
    else()
      message(FATAL_ERROR "yyjson target not found after FetchContent")
    endif()
  endif()
  add_library(zerialize_yyjson ALIAS ${_zerialize_yyjson_target})
  target_link_libraries(${PROJECT_NAME} INTERFACE zerialize_yyjson)
  target_compile_definitions(${PROJECT_NAME} INTERFACE ZERIALIZE_HAS_JSON=1)
endif()

# Protocol: jsoncons (CBOR)
if(ZERIALIZE_ENABLE_CBOR)
  # Prefer an existing target
  if(TARGET jsoncons)
    target_link_libraries(${PROJECT_NAME} INTERFACE jsoncons)
  elseif(TARGET jsoncons::jsoncons)
    target_link_libraries(${PROJECT_NAME} INTERFACE jsoncons::jsoncons)
  else()
    # Vendor header-only jsoncons
    if(NOT TARGET jsoncons AND NOT TARGET jsoncons::jsoncons)
      FetchContent_Declare(
          jsoncons
          GIT_REPOSITORY https://github.com/danielaparker/jsoncons.git
          GIT_TAG v1.4.0
      )
      FetchContent_MakeAvailable(jsoncons)
    endif()
    if(TARGET jsoncons)
      target_link_libraries(${PROJECT_NAME} INTERFACE jsoncons)
    elseif(TARGET jsoncons::jsoncons)
      target_link_libraries(${PROJECT_NAME} INTERFACE jsoncons::jsoncons)
    elseif(DEFINED jsoncons_SOURCE_DIR)
      target_include_directories(${PROJECT_NAME} INTERFACE ${jsoncons_SOURCE_DIR}/include)
    else()
      message(FATAL_ERROR "jsoncons not available for CBOR support")
    endif()
  endif()
  target_compile_definitions(${PROJECT_NAME} INTERFACE ZERIALIZE_HAS_CBOR=1)
endif()

# Protocol: msgpack-c (MessagePack)
if(ZERIALIZE_ENABLE_MSGPACK)
  # Reuse if present; otherwise vendor
  if(NOT TARGET msgpack-c AND NOT TARGET msgpackc-cxx AND NOT TARGET msgpackc)
    FetchContent_Declare(
        msgpack
        GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
        GIT_TAG c_master
    )
    FetchContent_GetProperties(msgpack)
    if(NOT msgpack_POPULATED)
        FetchContent_Populate(msgpack)
        # Disable Boost / samples / tests
        set(MSGPACK_BOOST OFF CACHE BOOL "Disable Boost" FORCE)
        set(MSGPACK_USE_BOOST OFF CACHE BOOL "Disable Boost" FORCE)
        set(MSGPACK_BUILD_EXAMPLES OFF CACHE BOOL "Don't build examples" FORCE)
        set(MSGPACK_BUILD_TESTS OFF CACHE BOOL "Don't build tests" FORCE)
        add_subdirectory(${msgpack_SOURCE_DIR} ${msgpack_BINARY_DIR})
    endif()
  endif()

  if(TARGET msgpack-c)
    add_library(zerialize_msgpack ALIAS msgpack-c)
  elseif(TARGET msgpackc-cxx)
    add_library(zerialize_msgpack ALIAS msgpackc-cxx)
  elseif(TARGET msgpackc)
    add_library(zerialize_msgpack ALIAS msgpackc)
  else()
    message(FATAL_ERROR "Could not locate a msgpack CMake target (msgpack-c/msgpackc-cxx/msgpackc).")
  endif()

  target_link_libraries(${PROJECT_NAME} INTERFACE zerialize_msgpack)
  target_compile_definitions(${PROJECT_NAME} INTERFACE ZERIALIZE_HAS_MSGPACK=1)
endif()

# -------------------
# Math backends: xtensor and Eigen

if(ZERIALIZE_ENABLE_XTENSOR)
  # If a toolchain already provides xtensor target, reuse it
  if(NOT TARGET xtensor)
    # xtensor depends on xtl and optionally xsimd
    FetchContent_Declare(xtl
        URL https://github.com/xtensor-stack/xtl/archive/0.8.0.zip
    )
    FetchContent_MakeAvailable(xtl)

    FetchContent_Declare(xsimd
        URL https://github.com/xtensor-stack/xsimd/archive/13.2.0.zip
    )
    FetchContent_MakeAvailable(xsimd)

    FetchContent_Declare(xtensor
        URL https://github.com/xtensor-stack/xtensor/archive/0.27.0.zip
    )
    set(XTENSOR_USE_XSIMD 1)
    FetchContent_MakeAvailable(xtensor)
  endif()
  target_link_libraries(${PROJECT_NAME} INTERFACE xtensor)
  target_compile_definitions(${PROJECT_NAME} INTERFACE ZERIALIZE_USE_XTENSOR=1)
endif()

if(ZERIALIZE_ENABLE_EIGEN)
  # Try to reuse Eigen3 if available, otherwise vendor a minimal interface
  find_package(Eigen3 QUIET CONFIG)
  if(TARGET Eigen3::Eigen)
    target_link_libraries(${PROJECT_NAME} INTERFACE Eigen3::Eigen)
  else()
    FetchContent_Declare(eigen_download
        URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.zip
        URL_HASH SHA256=eba3f3d414d2f8cba2919c78ec6daab08fc71ba2ba4ae502b7e5d4d99fc02cda
        GIT_TAG        origin/master
    )
    FetchContent_GetProperties(eigen_download)
    if(NOT eigen_download_POPULATED)
        FetchContent_Populate(eigen_download)
    endif()
    add_library(eigen INTERFACE IMPORTED)
    target_include_directories(eigen INTERFACE ${eigen_download_SOURCE_DIR})
    target_link_libraries(${PROJECT_NAME} INTERFACE eigen)
  endif()
  target_compile_definitions(${PROJECT_NAME} INTERFACE ZERIALIZE_USE_EIGEN=1)
endif()

if(ZERIALIZE_ENABLE_MODULES)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
        message(STATUS "Building zerialize C++ module")
        add_subdirectory(modules)
    else()
        message(WARNING "Skipping zerialize C++ module (requires CMake 3.28+, found ${CMAKE_VERSION})")
    endif()
endif()
