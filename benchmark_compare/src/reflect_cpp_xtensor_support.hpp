// #ifndef XTENSOR_RFL_HPP
// #define XTENSOR_RFL_HPP

// #include <xtensor/containers/xarray.hpp>
// #include <rfl.hpp> // Main reflect-cpp header

// #include <vector>
// #include <string>  // For error messages
// #include <stdexcept>
// #include <cstring>      // For memcpy
// #include <type_traits>  // For type checks
// #include <cstdint>      // For specific integer types
// #include <limits>       // For numeric_limits
// #include <optional>     // For reverse lookup result

// namespace xtensor_rfl {

// // --- Get integer dtype code from C++ type ---
// template <typename T>
// constexpr int get_dtype_code() {
//     static_assert(std::is_trivial_v<T> && std::is_standard_layout_v<T>,
//                   "xtensor element type must be trivial and standard layout for safe binary serialization");

//     // Use -1 as a sentinel for unsupported types
//     int code = -1;

//     if constexpr (std::is_same_v<T, int8_t>)   code = 0;
//     else if constexpr (std::is_same_v<T, int16_t>)  code = 1;
//     else if constexpr (std::is_same_v<T, int32_t>)  code = 2;
//     else if constexpr (std::is_same_v<T, int64_t>)  code = 3;
//     else if constexpr (std::is_same_v<T, uint8_t>)  code = 4;
//     else if constexpr (std::is_same_v<T, uint16_t>) code = 5;
//     else if constexpr (std::is_same_v<T, uint32_t>) code = 6;
//     else if constexpr (std::is_same_v<T, uint64_t>) code = 7;
//     else if constexpr (std::is_same_v<T, float>)    code = 10;
//     else if constexpr (std::is_same_v<T, double>)   code = 11;
//     // Add bool or other types here if needed, mapping them to integer codes

//     return code;
// }

// // --- Helper to get type properties from integer code (for validation) ---
// struct DTypeCodeInfo {
//     std::size_t size;
//     std::string name;
// };

// inline std::optional<DTypeCodeInfo> get_dtype_info_from_code(int code) {
//     switch (code) {
//         case 0: return DTypeCodeInfo{sizeof(int8_t), "int8"};
//         case 1: return DTypeCodeInfo{sizeof(int16_t), "int16"};
//         case 2: return DTypeCodeInfo{sizeof(int32_t), "int32"};
//         case 3: return DTypeCodeInfo{sizeof(int64_t), "int64"};
//         case 4: return DTypeCodeInfo{sizeof(uint8_t), "uint8"};
//         case 5: return DTypeCodeInfo{sizeof(uint16_t), "uint16"};
//         case 6: return DTypeCodeInfo{sizeof(uint32_t), "uint32"};
//         case 7: return DTypeCodeInfo{sizeof(uint64_t), "uint64"};
//         case 10: return DTypeCodeInfo{sizeof(float), "float32"};
//         case 11: return DTypeCodeInfo{sizeof(double), "float64"};
//         // Add bool etc. if mapped
//         default: return std::nullopt; // Invalid or unknown code
//     }
// }


// // --- The Intermediate Structure for Reflection ---
// // Defined within the xtensor_rfl namespace
// struct XtensorReflectionData {
//     rfl::Field<"shape", std::vector<std::size_t>> shape = rfl::default_value;
//     rfl::Field<"dtype", int> dtype = rfl::default_value;
//     rfl::Field<"data", std::vector<std::uint8_t>> data = rfl::default_value;
// };

// // RFL_STRUCT IS MOVED OUTSIDE THE NAMESPACE BLOCK BELOW

// } // namespace xtensor_rfl


// // --- Define reflection for the intermediate struct in the GLOBAL namespace ---
// // Use the fully qualified name of the struct.
// //RFL_STRUCT(xtensor_rfl::XtensorReflectionData, shape, dtype, data)


// // --- rfl::Reflector Specialization ---
// // This MUST be in the rfl namespace.
// namespace rfl {

// template <typename T> // Template for the element type of the xarray
// struct Reflector<xt::xarray<T>> {
//     // 1. Define the reflection type (`ReflType`)
//     //    Use the fully qualified name here too for clarity
//     using ReflType = xtensor_rfl::XtensorReflectionData;

//     // 2. Implement `from`: Convert xt::xarray<T> to ReflType
//     static ReflType from(const xt::xarray<T>& tensor) noexcept {
//         ReflType reflected_data; // Default constructible now

//         try {
//             const int dtype_code = xtensor_rfl::get_dtype_code<T>();

//             // Check if type is supported
//             if (dtype_code == -1) {
//                 return ReflType{}; // Return default/empty
//             }

//             reflected_data.shape = std::vector<std::size_t>(tensor.shape().begin(), tensor.shape().end());
//             reflected_data.dtype = dtype_code; // Assign the integer code

//             const size_t byte_size = tensor.size() * sizeof(T);
//             reflected_data.data().resize(byte_size); // Access via ()

//             if (byte_size > 0) {
//                 std::memcpy(reflected_data.data().data(), tensor.data(), byte_size); // Access via ()
//             }
//             return reflected_data;

//         } catch (...) {
//             // Fallback for unexpected errors
//             return ReflType{};
//         }
//     }

//     // 3. Implement `to`: Convert ReflType back to xt::xarray<T>
//     static Result<xt::xarray<T>> to(const ReflType& v) noexcept {
//         try {
//             // Get expected and received codes
//             const int expected_code = xtensor_rfl::get_dtype_code<T>();
//             const int received_code = v.dtype(); // Access int via ()

//             // Check if the target type T is supported
//             if (expected_code == -1) {
//                  return Error("Cannot deserialize into an unknown/unsupported xtensor type (code -1).");
//             }

//             // Get info about the received code for validation
//             auto received_info_opt = xtensor_rfl::get_dtype_info_from_code(received_code);
//             if (!received_info_opt) {
//                  return Error("Received unknown or invalid xtensor dtype code during deserialization: " +
//                             std::to_string(received_code));
//             }
//             const auto& received_info = *received_info_opt;

//             // Primary check: Does the received code match the expected code for type T?
//             if (received_code != expected_code) {
//                  auto expected_info = xtensor_rfl::get_dtype_info_from_code(expected_code);
//                  std::string expected_name = expected_info ? expected_info->name : "UNKNOWN";
//                  return Error("xtensor type mismatch during deserialization. Expected code " +
//                             std::to_string(expected_code) + " (" + expected_name +
//                             "), but received code " + std::to_string(received_code) +
//                             " (" + received_info.name + ").");
//             }

//             // Secondary check (sanity): Does sizeof(T) match the size implied by the code?
//             if (sizeof(T) != received_info.size) {
//                 return Error("Internal inconsistency: sizeof(T) [" + std::to_string(sizeof(T)) +
//                            "] does not match size implied by received dtype code " +
//                            std::to_string(received_code) + " [" + std::to_string(received_info.size) + "]");
//             }


//             // Validate shape and data size
//             size_t expected_elements = 1;
//             const auto& shape_vec = v.shape(); // Access via ()
//             for (size_t dim : shape_vec) {
//                  // Basic overflow check for element count
//                  if (std::numeric_limits<size_t>::max() / dim < expected_elements) {
//                      return Error("Potential overflow calculating total elements from shape.");
//                  }
//                  expected_elements *= dim;
//             }
//             // Use the size derived from the received code info for validation
//             const size_t expected_byte_size = expected_elements * received_info.size;
//              // Check for overflow in byte size calculation
//              if (expected_elements > 0 && std::numeric_limits<size_t>::max() / expected_elements < received_info.size) {
//                  return Error("Potential overflow calculating total byte size.");
//              }


//             // Access data via ()
//             if (v.data().size() != expected_byte_size) {
//                  return Error("xtensor data size mismatch during deserialization. Expected " +
//                             std::to_string(expected_byte_size) + " bytes for shape and dtype code " +
//                             std::to_string(received_code) + ", but received " +
//                             std::to_string(v.data().size()) + " bytes.");
//             }

//             // Create the tensor (uninitialized)
//             xt::xarray<T> tensor = xt::xarray<T>::from_shape(v.shape()); // Access via ()

//             // Copy data if non-empty
//             if (expected_byte_size > 0) {
//                  std::memcpy(tensor.data(), v.data().data(), expected_byte_size); // Access via ()
//             }

//             return tensor; // Success

//         } catch (const std::exception& e) {
//              // Catch exceptions from std::vector, std::memcpy, etc.
//              return Error(std::string("Exception during xtensor deserialization: ") + e.what());
//         } catch(...) {
//              return Error("Unknown exception during xtensor deserialization.");
//         }
//     }
// };

// } // namespace rfl

// #endif // XTENSOR_RFL_HPP