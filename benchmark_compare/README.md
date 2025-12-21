# benchmark_compare

Compares zerialize vs reflect-cpp.

Also benchmarks zerialize's built-in `Zer` protocol, which is dependency-free and has no reflect-cpp equivalent (so the `Zer` section reports only `Zerialize` rows).

## Build

    cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake
    cmake --build build --config Release

## Build for Profiling

    cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake
    cmake --build build

## Run

    ./build/benchmark_compare

## Results

```
Serialize:    produce bytes
Deserialize:  consume bytes
Read:         read and check every value from pre-deserialized, read single tensor element
Deser+Read:   deserialize, then read

--- Json                Serialize (µs)  Deserialize (µs)         Read (µs)   Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                    0.507             0.126             0.142             0.268               105           1000000
    ReflectCpp                   0.345             0.209             0.022             0.231               105           1000000

SmallStructAsVector
    Zerialize                    0.313             0.144             0.123             0.267                49           1000000
    ReflectCpp                   0.250             0.222             0.022             0.244                49           1000000



--- Flex                Serialize (µs)  Deserialize (µs)         Read (µs)   Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                    0.849             0.000             0.127             0.127               151           1000000
    ReflectCpp                   0.879             0.105             0.022             0.127               151           1000000

SmallStructAsVector
    Zerialize                    0.506             0.000             0.080             0.080                87           1000000
    ReflectCpp                   0.586             0.099             0.022             0.121                87           1000000

SmallTensorStruct 4x4 double
    Zerialize                    1.157             0.000             0.462             0.462               320           1000000
    ReflectCpp                   1.115             0.505             0.272             0.777               320           1000000

SmallTensorStructAsVector 4x4 double
    Zerialize                    0.669             0.000             0.393             0.393               232           1000000
    ReflectCpp                   0.675             0.495             0.275             0.770               232           1000000

MediumTensorStruct 1x2048 float
    Zerialize                    1.438             0.000             0.469             0.469              8400            100000
    ReflectCpp                   1.496             0.588             0.396             0.984              8400            100000

MediumTensorStructAsVector 1x2048 float
    Zerialize                    0.982             0.000             0.405             0.405              8304            100000
    ReflectCpp                   1.064             0.576             0.388             0.964              8304            100000

LargeTensorStruct 3x1024x768 uint8
    Zerialize                  262.999             0.000             0.497             0.497           2359528             10000
    ReflectCpp                 601.732           133.397           125.070           258.467           2359424             10000



--- MsgPack             Serialize (µs)  Deserialize (µs)         Read (µs)   Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                    0.143             0.000             0.223             0.223                82           1000000
    ReflectCpp                   0.138             0.167             0.022             0.189                82           1000000

SmallStructAsVector
    Zerialize                    0.109             0.000             0.181             0.181                34           1000000
    ReflectCpp                   0.107             0.147             0.022             0.169                34           1000000

SmallTensorStruct 4x4 double
    Zerialize                    0.232             0.000             0.674             0.674               230           1000000
    ReflectCpp                   0.184             0.568             0.267             0.834               230           1000000

SmallTensorStructAsVector 4x4 double
    Zerialize                    0.185             0.000             0.611             0.611               169           1000000
    ReflectCpp                   0.139             0.570             0.322             0.893               169           1000000

MediumTensorStruct 1x2048 float
    Zerialize                    0.637             0.000             1.349             1.349              8297            100000
    ReflectCpp                   1.036             1.381             0.977             2.358              8297            100000

MediumTensorStructAsVector 1x2048 float
    Zerialize                    0.565             0.000             0.648             0.648              8236            100000
    ReflectCpp                   0.423             0.687             0.361             1.048              8236            100000

LargeTensorStruct 3x1024x768 uint8
    Zerialize                  137.853             0.000             0.745             0.745           2359406             10000
    ReflectCpp                 280.299           135.388           130.071           265.459           2359345             10000



--- CBOR                Serialize (µs)  Deserialize (µs)         Read (µs)   Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                    0.606             0.000             0.576             0.576                83           1000000
    ReflectCpp                   0.637             1.292             0.022             1.314                84           1000000

SmallStructAsVector
    Zerialize                    0.447             0.000             0.419             0.419                35           1000000
    ReflectCpp                   0.499             0.907             0.022             0.929                35           1000000

SmallTensorStruct 4x4 double
    Zerialize                    1.068             0.000             1.231             1.231               231           1000000
    ReflectCpp                   1.036             2.263             0.288             2.550               232           1000000

SmallTensorStructAsVector 4x4 double
    Zerialize                    0.899             0.000             0.952             0.952               170           1000000
    ReflectCpp                   0.873             1.789             0.283             2.073               170           1000000

MediumTensorStruct 1x2048 float
    Zerialize                   16.952             0.000             1.229             1.229              8298            100000
    ReflectCpp                  16.931             2.795             0.358             3.152              8299            100000

MediumTensorStructAsVector 1x2048 float
    Zerialize                   16.751             0.000             0.951             0.951              8237            100000
    ReflectCpp                  16.676             2.320             0.377             2.697              8237            100000

LargeTensorStruct 3x1024x768 uint8
    Zerialize                 5119.022             0.000             1.276             1.276           2359407             10000
    ReflectCpp                5158.345           999.197           128.862          1128.059           2359346             10000

```
*Zerialize has support for tensors (with blobs via base64 encoded strings) in json, but reflect doesn't, so don't even try.*
