# benchmarking code for zerialize

## Build

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

## Current results

```
Flex Serializer:
Test Name                                Serialization (µs)    Deserialization (µs)      Data Size (bytes)
----------------------------------------------------------------------------------------------------------
Small: Single Int                                     0.190                   0.002                      3
Small: Int, Double, String                            0.275                   0.128                     46
Medium: Map with Nested Values                        1.169                   0.803                    127
Medium: Small XTensor (2x3)                           1.247                   1.251                    123
Medium: Small Eigen Matrix (3x3)                      2.493                   2.299                    147
Large: XTensor (20x20)                                2.820                   2.560                   3290
Large: Eigen Matrix (20x20)                           2.900                   2.070                   3290
Very large: XTensor (3x640x480)                    1172.250                   1.530                7372926

JSON Serializer:
Test Name                                Serialization (µs)    Deserialization (µs)      Data Size (bytes)
----------------------------------------------------------------------------------------------------------
Small: Single Int                                     0.204                   0.001                      2
Small: Int, Double, String                            0.520                   0.047                     17
Medium: Map with Nested Values                        1.886                   0.385                     81
Medium: Small XTensor (2x3)                           2.249                   2.112                    133
Medium: Small Eigen Matrix (3x3)                      2.553                   1.777                    165
Large: XTensor (20x20)                               20.320                   6.360                   4339
Large: Eigen Matrix (20x20)                          21.100                   6.250                   4339
Very large: XTensor (3x640x480)                   45858.140               11993.400                9830475

MsgPack Serializer:
Test Name                                Serialization (µs)    Deserialization (µs)      Data Size (bytes)
----------------------------------------------------------------------------------------------------------
Small: Single Int                                     0.255                   0.001                      1
Small: Int, Double, String                            0.245                   0.059                     17
Medium: Map with Nested Values                        0.706                   0.321                     58
Medium: Small XTensor (2x3)                           0.870                   0.883                     98
Medium: Small Eigen Matrix (3x3)                      0.953                   0.635                    122
Large: XTensor (20x20)                                1.080                   0.990                   3251
Large: Eigen Matrix (20x20)                           1.620                   0.700                   3251
Very large: XTensor (3x640x480)                    1477.290                   0.920                7372858
```