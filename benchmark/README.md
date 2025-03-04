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
Null                                           0.055            0.000            0.000                0        1000000
Small: Single Int                              0.077            0.000            0.002                3        1000000
Small: Int, Double, String                     0.230            0.000            0.019               46        1000000
Medium: Map with Nested Values                 1.096            0.000            0.143              127        1000000
Medium: Small XTensor (2x3)                    1.171            0.000            0.757              123        1000000
Medium: Small Eigen Matrix (3x3)               1.209            0.000            0.481              147        1000000
Large: XTensor (20x20)                         1.278            0.000            0.760             3290           1000
Large: Eigen Matrix (20x20)                    1.330            0.000            0.501             3290           1000
Very large: XTensor (3x640x480)               73.157            0.000            0.799           921726           1000

JSON Serializer:
Test Name                             Serialize (µs) Deserialize (µs)        Read (µs) Data Size (bytes)     (samples)
----------------------------------------------------------------------------------------------------------------------
Function call only baseline                    0.000            0.000            0.000          3000000        1000000
Null                                           0.208            0.165            0.000                4        1000000
Small: Single Int                              0.211            0.164            0.002                2        1000000
Small: Int, Double, String                     0.786            0.669            0.041               17        1000000
Medium: Map with Nested Values                 2.666            1.825            0.376               81        1000000
Medium: Small XTensor (2x3)                    3.705            2.052            1.954              133        1000000
Medium: Small Eigen Matrix (3x3)               3.386            2.226            1.779              165        1000000
Large: XTensor (20x20)                        24.170           19.225            7.500             4339           1000
Large: Eigen Matrix (20x20)                   24.670           18.982            7.219             4339           1000
Very large: XTensor (3x640x480)             6519.578         5126.119         1673.905          1228874           1000

MsgPack Serializer:
Test Name                             Serialize (µs) Deserialize (µs)        Read (µs) Data Size (bytes)     (samples)
----------------------------------------------------------------------------------------------------------------------
Function call only baseline                    0.000            0.000            0.000          3000000        1000000
Null                                           0.019            0.002            0.000                0        1000000
Small: Single Int                              0.871            0.124            0.002                1        1000000
Small: Int, Double, String                     0.899            0.144            0.013               17        1000000
Medium: Map with Nested Values                 1.544            0.217            0.079               58        1000000
Medium: Small XTensor (2x3)                    1.666            0.226            0.600               98        1000000
Medium: Small Eigen Matrix (3x3)               1.646            0.224            0.349              122        1000000
Large: XTensor (20x20)                         1.638            0.253            0.580             3251           1000
Large: Eigen Matrix (20x20)                    1.670            0.264            0.346             3251           1000
Very large: XTensor (3x640x480)              181.606           15.972            0.647           921658           1000
```