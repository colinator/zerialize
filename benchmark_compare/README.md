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

## Results

Serialize:    produce bytes
Deserialize:  consume bytes
Read:         read and check every value from pre-deserialized, compute tensor element sums
Deser+Read:   deserialize, then read

--- Flex                Serialize (µs)  Deserialize (µs)         Read (µs)   Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                    0.881             0.000             0.174             0.174               151           1000000
    ReflectCpp                   0.917             0.312             0.072             0.384               151           1000000

SmallStructAsVector
    Zerialize                    0.494             0.000             0.089             0.089                87           1000000
    ReflectCpp                   0.599             0.317             0.072             0.389                87           1000000

SmallTensorStruct 4x4 double
    Zerialize                    1.176             0.000             0.591             0.591               320           1000000
    ReflectCpp                   1.189             0.496             0.315             0.812               320           1000000

SmallTensorStructAsVector 4x4 double
    Zerialize                    0.661             0.000             0.470             0.470               232           1000000
    ReflectCpp                   0.674             0.495             0.332             0.826               232           1000000

MediumTensorStruct 10x2048 float
    Zerialize                    5.314             0.000            17.870            17.870             82144            100000
    ReflectCpp                   7.375             1.733            19.051            20.784             82144            100000

MediumTensorStructAsVector 10x2048 float
    Zerialize                    4.710             0.000            17.853            17.853             82048            100000
    ReflectCpp                   7.236             1.352            19.220            20.572             82048            100000

LargeTensorStruct 3x1024x768 uint8
    Zerialize                  274.595             0.000           782.198           782.198           2359528             10000
    ReflectCpp                 632.296           140.830           917.750          1058.579           2359424             10000



--- MsgPack             Serialize (µs)  Deserialize (µs)         Read (µs)   Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                    0.145             0.000             0.233             0.233                82           1000000
    ReflectCpp                   0.152             0.387             0.071             0.458                82           1000000

SmallStructAsVector
    Zerialize                    0.112             0.000             0.186             0.186                34           1000000
    ReflectCpp                   0.116             0.364             0.066             0.431                34           1000000

SmallTensorStruct 4x4 double
    Zerialize                    0.232             0.000             0.747             0.747               230           1000000
    ReflectCpp                   0.187             0.603             0.321             0.924               230           1000000

SmallTensorStructAsVector 4x4 double
    Zerialize                    0.190             0.000             0.685             0.685               169           1000000
    ReflectCpp                   0.143             0.569             0.321             0.890               169           1000000

MediumTensorStruct 10x2048 float
    Zerialize                    1.865             0.000            21.627            21.627             82027            100000
    ReflectCpp                   7.361             1.894            19.727            21.621             82027            100000

MediumTensorStructAsVector 10x2048 float
    Zerialize                    1.783             0.000            18.006            18.006             81966            100000
    ReflectCpp                   5.003             1.893            19.132            21.024             81966            100000

LargeTensorStruct 3x1024x768 uint8
    Zerialize                  132.221             0.000           786.471           786.471           2359406             10000
    ReflectCpp                 279.149           130.790           899.323          1030.113           2359345             10000



--- Json                Serialize (µs)  Deserialize (µs)         Read (µs)   Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                    0.518             0.131             0.172             0.302               105           1000000
    ReflectCpp                   0.353             0.459             0.067             0.526               105           1000000

SmallStructAsVector
    Zerialize                    0.334             0.146             0.127             0.273                49           1000000
    ReflectCpp                   0.254             0.461             0.073             0.534                49           1000000

    