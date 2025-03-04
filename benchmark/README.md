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
Test Name                               Serialize (µs)   Deserialize (µs)          Read (µs)   Data Size (bytes)
----------------------------------------------------------------------------------------------------------------
Function call only baseline                      0.000              0.000              0.000               20000
Null                                             0.052              0.042              0.000                   0
Small: Single Int                                0.134              0.055              0.002                   3
Small: Int, Double, String                       0.601              0.099              0.268                  46
Medium: Map with Nested Values                   2.067              0.070              1.064                 127
Medium: Small XTensor (2x3)                      1.525              0.063              1.474                 123
Medium: Small Eigen Matrix (3x3)                 1.382              0.055              1.042                 147
Large: XTensor (20x20)                           1.365              0.048              1.330                3290
Large: Eigen Matrix (20x20)                      1.410              0.044              1.022                3290
Very large: XTensor (3x640x480)                 87.471              0.042              1.270              921726

JSON Serializer:
Test Name                               Serialize (µs)   Deserialize (µs)          Read (µs)   Data Size (bytes)
----------------------------------------------------------------------------------------------------------------
Function call only baseline                      0.000              0.000              0.000               20000
Null                                             0.148              0.149              0.000                   4
Small: Single Int                                0.154              0.153              0.002                   2
Small: Int, Double, String                       0.528              0.650              0.039                  17
Medium: Map with Nested Values                   1.854              1.647              0.366                  81
Medium: Small XTensor (2x3)                      2.340              1.799              1.882                 133
Medium: Small Eigen Matrix (3x3)                 2.459              1.970              1.693                 165
Large: XTensor (20x20)                          20.569             17.509              6.350                4339
Large: Eigen Matrix (20x20)                     20.020             17.320              6.293                4339
Very large: XTensor (3x640x480)               5479.200           4623.025           1392.264             1228874

MsgPack Serializer:
Test Name                               Serialize (µs)   Deserialize (µs)          Read (µs)   Data Size (bytes)
----------------------------------------------------------------------------------------------------------------
Function call only baseline                      0.000              0.000              0.000               20000
Null                                             0.020              0.021              0.000                   0
Small: Single Int                                0.204              0.159              0.001                   1
Small: Int, Double, String                       0.247              0.168              0.064                  17
Medium: Map with Nested Values                   0.717              0.230              0.329                  58
Medium: Small XTensor (2x3)                      0.906              0.233              0.868                  98
Medium: Small Eigen Matrix (3x3)                 0.938              0.237              0.642                 122
Large: XTensor (20x20)                           0.984              0.270              0.950                3251
Large: Eigen Matrix (20x20)                      1.080              0.276              0.683                3251
Very large: XTensor (3x640x480)                 89.859             14.821              0.959              921658
```