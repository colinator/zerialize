#pragma once

// Core components - essential for most use cases
#include "errors.hpp"
#include "concepts.hpp"
#include "value_type.hpp"
#include "deserializer.hpp"
#include "serializer.hpp"

// API components - user-facing functionality
#include "api.hpp"

// Utilities - commonly used
#include "debug_utils.hpp"

// Note: Advanced users can include individual headers for faster compilation
// Example: #include <zerialize/serializer.hpp> for serialization-only code
//
// Protocol-specific headers are available in protocols/ subdirectory:
// #include <zerialize/protocols/json.hpp>
// #include <zerialize/protocols/msgpack.hpp>
// #include <zerialize/protocols/flex.hpp>
//
// Tensor library integrations are available in tensor/ subdirectory:
// #include <zerialize/tensor/eigen.hpp>
// #include <zerialize/tensor/xtensor.hpp>
