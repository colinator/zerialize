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

# Fetch nlohmann json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Fetch msgpack-c (MessagePack for C/C++)
FetchContent_Declare(
    msgpack
    GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
    GIT_TAG c_master
)
FetchContent_GetProperties(msgpack)
if(NOT msgpack_POPULATED)
    FetchContent_Populate(msgpack)
    # Explicitly disable Boost
    set(MSGPACK_BOOST OFF CACHE BOOL "Disable Boost dependency" FORCE)
    set(MSGPACK_USE_BOOST OFF CACHE BOOL "Disable Boost dependency" FORCE)
    set(MSGPACK_BUILD_EXAMPLES OFF CACHE BOOL "Don't build examples" FORCE)
    set(MSGPACK_BUILD_TESTS OFF CACHE BOOL "Don't build tests" FORCE)
    add_subdirectory(${msgpack_SOURCE_DIR} ${msgpack_BINARY_DIR})
endif()

# Download and configure xtl
FetchContent_Declare(xtl
    URL https://github.com/xtensor-stack/xtl/archive/0.7.5.zip
    URL_HASH SHA256=9e8e7dcc525500a4543226fe472a46a30636ee35274e4e30099071c5cbd4d05c
    DOWNLOAD_NO_PROGRESS ON
    DOWNLOAD_EXTRACT_TIMESTAMP 1
)
FetchContent_MakeAvailable(xtl)

# Download and configure xsimd
FetchContent_Declare(xsimd
    URL https://github.com/xtensor-stack/xsimd/archive/11.0.0.zip
    URL_HASH SHA256=5d362ec26c6f2332d1a2858891d770f5c0077133a81f885658f48c910a03fc90
    DOWNLOAD_NO_PROGRESS ON
    DOWNLOAD_EXTRACT_TIMESTAMP 1
)
FetchContent_MakeAvailable(xsimd)

# Download and configure xtensor
FetchContent_Declare(xtensor
    URL https://github.com/xtensor-stack/xtensor/archive/0.24.6.zip
    URL_HASH SHA256=8cd062cfefce0f5bff8961e0c42b3636d5e7eddbe33e5c3167e3fb6d8380f50e
    DOWNLOAD_NO_PROGRESS ON
    DOWNLOAD_EXTRACT_TIMESTAMP 1
)
set(XTENSOR_USE_XSIMD 1)
FetchContent_MakeAvailable(xtensor)

# Download and configure eigen
FetchContent_Declare(eigen_download
    URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.zip
    URL_HASH SHA256=1ccaabbfe870f60af3d6a519c53e09f3dcf630207321dffa553564a8e75c4fc8
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
    ${PROJECT_SOURCE_DIR}/include
    ${flatbuffers_SOURCE_DIR}/include
    nlohmann_json::nlohmann_json
    ${msgpack_SOURCE_DIR}/include
    #$<INSTALL_INTERFACE:include/eigen3>
)

# Doesn't really 'link' - just how to do transitive
# includes in cmake...
target_link_libraries(${PROJECT_NAME} INTERFACE
    nlohmann_json::nlohmann_json
    flatbuffers
    xtensor 
    xsimd 
    xtl
    eigen
    msgpack-c
)
