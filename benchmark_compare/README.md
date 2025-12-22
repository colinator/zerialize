# benchmark_compare

Compares zerialize vs reflect-cpp.

Also benchmarks zerialize's built-in `Zera` protocol, which is dependency-free and has no reflect-cpp equivalent (so the `Zera` section reports only `Zerialize` rows).

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

--- Json                     Serialize (µs)   Deserialize (µs)          Read (µs)    Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                         0.496              0.129              0.152              0.281               105           1000000
    ReflectCpp                        0.350              0.235              0.023              0.258               105           1000000

SmallStructAsVector
    Zerialize                         0.327              0.169              0.131              0.301                49           1000000
    ReflectCpp                        0.262              0.233              0.023              0.256                49           1000000



--- Flex                     Serialize (µs)   Deserialize (µs)          Read (µs)    Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                         0.864              0.001              0.158              0.159               151           1000000
    ReflectCpp                        0.919              0.109              0.023              0.131               151           1000000

SmallStructAsVector
    Zerialize                         0.503              0.001              0.114              0.115                87           1000000
    ReflectCpp                        0.602              0.103              0.024              0.127                87           1000000

SmallTensorStruct 4x4 double
    Zerialize (aligned)               1.173              0.001              0.324              0.325               320           1000000
    Zerialize (mis)                   1.176              0.001              0.474              0.475               320           1000000
    ReflectCpp (aligned)              1.159              0.515              0.172              0.687               320           1000000
    ReflectCpp (mis)                  1.162              0.516              0.085              0.601               328           1000000

SmallTensorStructAsVector 4x4 double
    Zerialize (aligned)               0.676              0.001              0.299              0.300               232           1000000
    Zerialize (mis)                   0.691              0.001              0.439              0.440               232           1000000
    ReflectCpp (aligned)              0.685              0.504              0.179              0.683               232           1000000
    ReflectCpp (mis)                  0.686              0.503              0.088              0.591               240           1000000

MediumTensorStruct 1x2048 float
    Zerialize (aligned)               1.437              0.001              0.340              0.340              8400            100000
    Zerialize (mis)                   1.427              0.001              0.564              0.564              8400            100000
    ReflectCpp (aligned)              1.563              0.619              0.173              0.792              8400            100000
    ReflectCpp (mis)                  1.547              0.623              0.145              0.768              8400            100000

MediumTensorStructAsVector 1x2048 float
    Zerialize (aligned)               0.978              0.001              0.301              0.302              8304            100000
    Zerialize (mis)                   1.044              0.001              0.534              0.535              8304            100000
    ReflectCpp (aligned)              1.082              0.608              0.180              0.789              8304            100000
    ReflectCpp (mis)                  1.073              0.588              0.171              0.759              8312            100000

LargeTensorStruct 3x1024x768 uint8
    Zerialize (aligned)             302.419              0.001              0.337              0.338           2359528             10000
    Zerialize (mis)                 303.127              0.001              0.347              0.348           2359528             10000
    ReflectCpp (aligned)            737.219            166.455              0.134            166.589           2359424             10000
    ReflectCpp (mis)                750.993            158.802              0.133            158.935           2359424             10000



--- MsgPack                  Serialize (µs)   Deserialize (µs)          Read (µs)    Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                         0.152              0.001              0.238              0.239                82           1000000
    ReflectCpp                        0.146              0.176              0.023              0.199                82           1000000

SmallStructAsVector
    Zerialize                         0.111              0.001              0.189              0.190                34           1000000
    ReflectCpp                        0.112              0.153              0.023              0.176                34           1000000

SmallTensorStruct 4x4 double
    Zerialize (aligned)               0.243              0.001              0.484              0.485               230           1000000
    Zerialize (mis)                   0.249              0.001              0.643              0.644               230           1000000
    ReflectCpp (aligned)              0.195              0.618              0.183              0.800               230           1000000
    ReflectCpp (mis)                  0.193              0.618              0.087              0.705               238           1000000

SmallTensorStructAsVector 4x4 double
    Zerialize (aligned)               0.202              0.001              0.449              0.449               169           1000000
    Zerialize (mis)                   0.202              0.001              0.593              0.593               169           1000000
    ReflectCpp (aligned)              0.154              0.568              0.175              0.743               169           1000000
    ReflectCpp (mis)                  0.141              0.573              0.084              0.658               177           1000000

MediumTensorStruct 1x2048 float
    Zerialize (aligned)               0.557              0.001              0.499              0.500              8297            100000
    Zerialize (mis)                   0.542              0.001              0.715              0.716              8297            100000
    ReflectCpp (aligned)              0.521              0.716              0.189              0.906              8297            100000
    ReflectCpp (mis)                  0.562              0.812              0.192              1.005              8301            100000

MediumTensorStructAsVector 1x2048 float
    Zerialize (aligned)               0.521              0.001              0.446              0.446              8236            100000
    Zerialize (mis)                   0.485              0.001              0.694              0.694              8236            100000
    ReflectCpp (aligned)              0.451              0.692              0.171              0.864              8236            100000
    ReflectCpp (mis)                  0.422              0.696              0.190              0.887              8240            100000

LargeTensorStruct 3x1024x768 uint8
    Zerialize (aligned)             139.630              0.001              0.499              0.500           2359406             10000
    Zerialize (mis)                 146.158              0.001              0.533              0.533           2359406             10000
    ReflectCpp (aligned)            318.643            143.636              0.131            143.767           2359345             10000
    ReflectCpp (mis)                324.768            143.884              0.123            144.007           2359345             10000



--- CBOR                     Serialize (µs)   Deserialize (µs)          Read (µs)    Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                         0.634              0.001              0.592              0.592                83           1000000
    ReflectCpp                        0.666              1.315              0.023              1.339                84           1000000

SmallStructAsVector
    Zerialize                         0.471              0.001              0.440              0.441                35           1000000
    ReflectCpp                        0.523              0.926              0.023              0.949                35           1000000

SmallTensorStruct 4x4 double
    Zerialize (aligned)               1.139              0.001              0.937              0.938               231           1000000
    Zerialize (mis)                   1.125              0.001              1.097              1.097               231           1000000
    ReflectCpp (aligned)              1.091              2.330              0.180              2.511               232           1000000
    ReflectCpp (mis)                  1.101              2.317              0.089              2.406               240           1000000

SmallTensorStructAsVector 4x4 double
    Zerialize (aligned)               0.931              0.001              0.769              0.770               170           1000000
    Zerialize (mis)                   0.932              0.001              0.944              0.945               170           1000000
    ReflectCpp (aligned)              0.901              1.862              0.184              2.046               170           1000000
    ReflectCpp (mis)                  0.932              1.845              0.099              1.944               178           1000000

MediumTensorStruct 1x2048 float
    Zerialize (aligned)              17.865              0.001              0.936              0.937              8298            100000
    Zerialize (mis)                  17.728              0.001              1.192              1.193              8298            100000
    ReflectCpp (aligned)             17.704              2.930              0.193              3.123              8299            100000
    ReflectCpp (mis)                 17.631              2.908              0.143              3.052              8303            100000

MediumTensorStructAsVector 1x2048 float
    Zerialize (aligned)              17.478              0.001              0.771              0.772              8237            100000
    Zerialize (mis)                  17.805              0.001              1.052              1.053              8237            100000
    ReflectCpp (aligned)             17.497              2.400              0.192              2.592              8237            100000
    ReflectCpp (mis)                 17.584              2.391              0.141              2.532              8241            100000

LargeTensorStruct 3x1024x768 uint8
    Zerialize (aligned)            5238.072              0.001              1.085              1.086           2359407             10000
    Zerialize (mis)                5398.212              0.001              0.971              0.971           2359407             10000
    ReflectCpp (aligned)           5266.568           1077.730              0.131           1077.861           2359346             10000
    ReflectCpp (mis)               5293.345           1108.855              0.130           1108.985           2359346             10000



--- Zera                     Serialize (µs)   Deserialize (µs)          Read (µs)    Deser+Read (µs)      Size (bytes)         (samples)

SmallStruct
    Zerialize                         0.554              0.004              0.121              0.125               336           1000000

SmallStructAsVector
    Zerialize                         0.431              0.004              0.092              0.096               272           1000000

SmallTensorStruct 4x4 double
    Zerialize (aligned)               0.912              0.004              0.295              0.299               592           1000000
    Zerialize (mis)                   0.921              0.004              0.430              0.434               592           1000000

SmallTensorStructAsVector 4x4 double
    Zerialize (aligned)               0.843              0.004              0.268              0.272               512           1000000
    Zerialize (mis)                   0.847              0.004              0.413              0.417               512           1000000

MediumTensorStruct 1x2048 float
    Zerialize (aligned)               1.577              0.004              0.288              0.292              8656            100000
    Zerialize (mis)                   1.273              0.004              0.526              0.530              8656            100000

MediumTensorStructAsVector 1x2048 float
    Zerialize (aligned)               1.184              0.004              0.264              0.268              8576            100000
    Zerialize (mis)                   1.158              0.004              0.504              0.509              8576            100000

LargeTensorStruct 3x1024x768 uint8
    Zerialize (aligned)             369.130              0.004              0.284              0.288           2359776             10000
    Zerialize (mis)                 349.491              0.005              0.313              0.318           2359776             10000

```
*Zerialize has support for tensors (with blobs via base64 encoded strings) in json, but reflect doesn't, so don't even try.*
