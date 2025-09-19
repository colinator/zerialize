cmake_minimum_required(VERSION 3.15)

project(zerialize VERSION 1.0 LANGUAGES CXX)

include(FetchContent)

# Fetch flatbuffers
FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY https://github.com/google/flatbuffers.git
    GIT_TAG master
)
FetchContent_MakeAvailable(flatbuffers)

# Fetch yyjson
FetchContent_Declare(
    yyjson
    GIT_REPOSITORY https://github.com/ibireme/yyjson.git
    GIT_TAG 0.12.0
)
FetchContent_MakeAvailable(yyjson)

# Fetch or reuse jsoncons (header-only). Guard to avoid target name collisions
if(NOT TARGET jsoncons AND NOT TARGET jsoncons::jsoncons)
  FetchContent_Declare(
      jsoncons
      GIT_REPOSITORY https://github.com/danielaparker/jsoncons.git
      GIT_TAG v1.4.0
  )
  FetchContent_MakeAvailable(jsoncons)
endif()

# Fetch msgpack
# If some toolchain (vcpkg, system) already provided msgpack targets, reuse them.
if(NOT TARGET msgpack-c AND NOT TARGET msgpackc-cxx AND NOT TARGET msgpackc)
  # Vendor via FetchContent
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

# # Fetch libcbor
# FetchContent_Declare(
#     libcbor
#     GIT_REPOSITORY https://github.com/PJK/libcbor.git
#     GIT_TAG v0.13.0
# )
# FetchContent_MakeAvailable(libcbor)

# Normalize to a single name consumers can link against:
if(TARGET msgpack-c)
  add_library(zerialize_msgpack ALIAS msgpack-c)
elseif(TARGET msgpackc-cxx)
  add_library(zerialize_msgpack ALIAS msgpackc-cxx)
elseif(TARGET msgpackc)
  add_library(zerialize_msgpack ALIAS msgpackc)
else()
  message(FATAL_ERROR "Could not locate a msgpack CMake target (msgpack-c/msgpackc-cxx/msgpackc).")
endif()


# Download and configure xtl
FetchContent_Declare(xtl
    # URL https://github.com/xtensor-stack/xtl/archive/0.7.5.zip
    # URL_HASH SHA256=9e8e7dcc525500a4543226fe472a46a30636ee35274e4e30099071c5cbd4d05c
    URL https://github.com/xtensor-stack/xtl/archive/0.8.0.zip
    #URL_HASH SHA256=9e8e7dcc525500a4543226fe472a46a30636ee35274e4e30099071c5cbd4d05c
    #DOWNLOAD_NO_PROGRESS ON
    #DOWNLOAD_EXTRACT_TIMESTAMP 1
)
FetchContent_MakeAvailable(xtl)

# Download and configure xsimd
FetchContent_Declare(xsimd
    # URL https://github.com/xtensor-stack/xsimd/archive/11.0.0.zip
    # URL_HASH SHA256=5d362ec26c6f2332d1a2858891d770f5c0077133a81f885658f48c910a03fc90
    URL https://github.com/xtensor-stack/xsimd/archive/13.2.0.zip
    #URL_HASH SHA256=5d362ec26c6f2332d1a2858891d770f5c0077133a81f885658f48c910a03fc90
    #DOWNLOAD_NO_PROGRESS ON
    #DOWNLOAD_EXTRACT_TIMESTAMP 1
)
FetchContent_MakeAvailable(xsimd)

# Download and configure xtensor
FetchContent_Declare(xtensor
    # URL https://github.com/xtensor-stack/xtensor/archive/0.24.6.zip
    # URL_HASH SHA256=8cd062cfefce0f5bff8961e0c42b3636d5e7eddbe33e5c3167e3fb6d8380f50e
    # URL https://github.com/xtensor-stack/xtensor/archive/0.26.0.zip
    URL https://github.com/xtensor-stack/xtensor/archive/0.27.0.zip
    #URL_HASH SHA256=8cd062cfefce0f5bff8961e0c42b3636d5e7eddbe33e5c3167e3fb6d8380f50e
    #DOWNLOAD_NO_PROGRESS ON
    #DOWNLOAD_EXTRACT_TIMESTAMP 1
)
set(XTENSOR_USE_XSIMD 1)
FetchContent_MakeAvailable(xtensor)

# Download and configure eigen
FetchContent_Declare(eigen_download
    URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.zip
    URL_HASH SHA256=eba3f3d414d2f8cba2919c78ec6daab08fc71ba2ba4ae502b7e5d4d99fc02cda
    GIT_TAG        origin/master
)
# FetchContent_MakeAvailable(eigen)
FetchContent_GetProperties(eigen_download)
if(NOT eigen_download_POPULATED)
    FetchContent_Populate(eigen_download)
endif()
add_library(eigen INTERFACE IMPORTED)
target_include_directories(eigen INTERFACE ${eigen_download_SOURCE_DIR})


# -------------------
# Create header-only 'interface' library
add_library(${PROJECT_NAME} INTERFACE)

# Specify the C++ standard
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

# Include directories
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${flatbuffers_SOURCE_DIR}/include
    ${msgpack_SOURCE_DIR}/include
    ${yyjson_SOURCE_DIR}/src
    # libcbor exposes proper include dirs via its target; no need to add here
    #$<INSTALL_INTERFACE:include/eigen3>
    #${xtensor_SOURCE_DIR}/include
    ${xtensor_INCLUDE_DIRS}
)

# Propagate jsoncons headers (prefer an existing target if provided by toolchain)
if(TARGET jsoncons)
  target_link_libraries(${PROJECT_NAME} INTERFACE jsoncons)
elseif(TARGET jsoncons::jsoncons)
  target_link_libraries(${PROJECT_NAME} INTERFACE jsoncons::jsoncons)
elseif(DEFINED jsoncons_SOURCE_DIR)
  target_include_directories(${PROJECT_NAME} INTERFACE ${jsoncons_SOURCE_DIR}/include)
endif()

# Doesn't really 'link' - just how to do transitive
# includes in cmake...
target_link_libraries(${PROJECT_NAME} INTERFACE
    flatbuffers
    xtensor 
    xsimd 
    xtl
    eigen
    zerialize_msgpack
    yyjson
    # cbor
)
