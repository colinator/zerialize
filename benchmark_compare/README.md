# benchmark_compare

Compares zerialize vs reflect-cpp.

## Build

    cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake
    cmake --build build --config Release

## Build for Profiling

    cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake
    cmake --build build

## Run

    ./build/benchmark_compare
    