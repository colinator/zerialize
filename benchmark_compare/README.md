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

```
Serialize:    produce bytes
Deserialize:  consume bytes
Read:         read and check every value from pre-deserialized, read single tensor element
Deser+Read:   deserialize, then read

--- Json                Serialize (µs)  Deserialize (µs)         Read (µs)   Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                    0.504             0.125             0.144             0.269               105           1000000
    ReflectCpp                   0.350             0.209             0.022             0.232               105           1000000

SmallStructAsVector
    Zerialize                    0.325             0.146             0.126             0.272                49           1000000
    ReflectCpp                   0.261             0.231             0.022             0.253                49           1000000



--- Flex                Serialize (µs)  Deserialize (µs)         Read (µs)   Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                    0.865             0.000             0.152             0.152               151           1000000
    ReflectCpp                   0.912             0.107             0.022             0.129               151           1000000

SmallStructAsVector
    Zerialize                    0.511             0.000             0.088             0.088                87           1000000
    ReflectCpp                   0.607             0.101             0.023             0.124                87           1000000

SmallTensorStruct 4x4 double
    Zerialize                    1.185             0.000             0.506             0.506               320           1000000
    ReflectCpp                   1.159             0.506             0.274             0.780               320           1000000

SmallTensorStructAsVector 4x4 double
    Zerialize                    0.694             0.000             0.409             0.409               232           1000000
    ReflectCpp                   0.684             0.516             0.295             0.811               232           1000000

MediumTensorStruct 1x2048 float
    Zerialize                    1.463             0.000             0.509             0.509              8400            100000
    ReflectCpp                   1.560             0.617             0.363             0.980              8400            100000

MediumTensorStructAsVector 1x2048 float
    Zerialize                    0.999             0.000             0.429             0.429              8304            100000
    ReflectCpp                   1.099             0.619             0.415             1.034              8304            100000

LargeTensorStruct 3x1024x768 uint8
    Zerialize                  290.880             0.000             0.529             0.529           2359528             10000
    ReflectCpp                 652.614           134.533           125.927           260.460           2359424             10000



--- MsgPack             Serialize (µs)  Deserialize (µs)         Read (µs)   Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                    0.147             0.000             0.228             0.228                82           1000000
    ReflectCpp                   0.140             0.174             0.023             0.197                82           1000000

SmallStructAsVector
    Zerialize                    0.112             0.000             0.186             0.186                34           1000000
    ReflectCpp                   0.111             0.154             0.023             0.176                34           1000000

SmallTensorStruct 4x4 double
    Zerialize                    0.239             0.000             0.705             0.705               230           1000000
    ReflectCpp                   0.191             0.607             0.283             0.890               230           1000000

SmallTensorStructAsVector 4x4 double
    Zerialize                    0.197             0.000             0.631             0.631               169           1000000
    ReflectCpp                   0.144             0.579             0.292             0.871               169           1000000

MediumTensorStruct 1x2048 float
    Zerialize                    0.547             0.000             0.740             0.740              8297            100000
    ReflectCpp                   0.533             0.743             0.387             1.130              8297            100000

MediumTensorStructAsVector 1x2048 float
    Zerialize                    0.385             0.000             0.661             0.661              8236            100000
    ReflectCpp                   0.468             0.715             0.381             1.096              8236            100000

LargeTensorStruct 3x1024x768 uint8
    Zerialize                  140.438             0.000             0.764             0.764           2359406             10000
    ReflectCpp                 294.955           157.413           127.530           284.943           2359345             10000



--- CBOR                Serialize (µs)  Deserialize (µs)         Read (µs)   Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                    0.633             0.000             0.590             0.590                83           1000000
    ReflectCpp                   0.657             1.339             0.023             1.362                84           1000000

SmallStructAsVector
    Zerialize                    0.468             0.000             0.432             0.432                35           1000000
    ReflectCpp                   0.518             0.935             0.022             0.958                35           1000000

SmallTensorStruct 4x4 double
    Zerialize                    1.124             0.000             1.258             1.258               231           1000000
    ReflectCpp                   1.080             2.332             0.295             2.628               232           1000000

SmallTensorStructAsVector 4x4 double
    Zerialize                    0.942             0.000             0.994             0.994               170           1000000
    ReflectCpp                   0.912             1.819             0.296             2.116               170           1000000

MediumTensorStruct 1x2048 float
    Zerialize                   17.384             0.000             1.242             1.242              8298            100000
    ReflectCpp                  17.162             2.850             0.367             3.218              8299            100000

MediumTensorStructAsVector 1x2048 float
    Zerialize                   16.957             0.000             0.979             0.979              8237            100000
    ReflectCpp                  16.943             2.294             0.463             2.757              8237            100000

LargeTensorStruct 3x1024x768 uint8
    Zerialize                 5046.466             0.000             1.319             1.319           2359407             10000
    ReflectCpp                5084.731          1027.865           129.675          1157.540           2359346             10000

```
*Zerialize has support for tensors (with blobs via base64 encoded strings) in json, but reflect doesn't, so don't even try.*
