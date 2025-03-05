# benchmarking code for zerialize

    cd benchmark

## Build

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

## Build for Profiling

    cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
    cmake --build build

## Run

    ./build/benchmark_zerialize

## Current results

```
Flex Serializer:
Test Name                             Serialize (µs) Deserialize (µs)        Read (µs) Data Size (bytes)     (samples)
----------------------------------------------------------------------------------------------------------------------
Function call only baseline                    0.000            0.000            0.000          3000000        1000000
Null                                           0.056            0.000            0.000                0        1000000
Small: Single Int                              0.085            0.000            0.002                3        1000000
Small: Int, Double, String                     0.230            0.000            0.019               46        1000000
Medium: Map Nested Values                      1.086            0.000            0.141              127        1000000
Medium: Map XTensor (2x3)                      0.658            0.000            0.384              100        1000000
Medium: Map Eigen Matrix (3x3)                 0.682            0.000            0.103              124        1000000
Large: Map XTensor (20x20)                     0.816            0.000            0.390             3260           1000
Large: Map Eigen Matrix (20x20)                1.060            0.000            0.109             3260           1000
Very large: XTensor (3x640x480)               63.550            0.000            0.351           921638           1000

JSON Serializer:
Test Name                             Serialize (µs) Deserialize (µs)        Read (µs) Data Size (bytes)     (samples)
----------------------------------------------------------------------------------------------------------------------
Function call only baseline                    0.000            0.000            0.000          3000000        1000000
Null                                           0.200            0.162            0.000                4        1000000
Small: Single Int                              0.194            0.167            0.002                2        1000000
Small: Int, Double, String                     0.778            0.671            0.040               17        1000000
Medium: Map Nested Values                      2.632            1.756            0.369               81        1000000
Medium: Map XTensor (2x3)                      2.713            1.791            1.475              110        1000000
Medium: Map Eigen Matrix (3x3)                 2.862            2.037            1.262              142        1000000
Large: Map XTensor (20x20)                    23.678           20.200            6.920             4316           1000
Large: Map Eigen Matrix (20x20)               23.770           20.081            6.677             4316           1000
Very large: XTensor (3x640x480)             6486.149         5444.285         1596.155          1228818           1000

MsgPack Serializer:
Test Name                             Serialize (µs) Deserialize (µs)        Read (µs) Data Size (bytes)     (samples)
----------------------------------------------------------------------------------------------------------------------
Function call only baseline                    0.000            0.000            0.000          3000000        1000000
Null                                           0.019            0.002            0.000                0        1000000
Small: Single Int                              0.866            0.117            0.002                1        1000000
Small: Int, Double, String                     0.981            0.147            0.013               17        1000000
Medium: Map Nested Values                      1.607            0.215            0.080               58        1000000
Medium: Map XTensor (2x3)                      1.443            0.196            0.328               81        1000000
Medium: Map Eigen Matrix (3x3)                 1.434            0.193            0.066              105        1000000
Large: Map XTensor (20x20)                     1.707            0.224            0.330             3234           1000
Large: Map Eigen Matrix (20x20)                1.590            0.222            0.066             3234           1000
Very large: XTensor (3x640x480)              181.715           15.224            0.360           921615           1000
```