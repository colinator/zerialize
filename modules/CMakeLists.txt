cmake_minimum_required(VERSION 3.28)

add_library(zerialize_modules)

set(ZERIALIZE_MODULES
    zerialize.cppm
    protocols/cbor.cppm
    protocols/flex.cppm
    protocols/json.cppm
    protocols/msgpack.cppm
    tensor/eigen.cppm
    tensor/utils.cppm
    tensor/xtensor.cppm
)

if(NOT COMMAND configure_cpp_module_target)
    function(configure_cpp_module_target target)
        target_sources(${target} PUBLIC FILE_SET CXX_MODULES FILES ${ZERIALIZE_MODULES})
    endfunction()
endif()

configure_cpp_module_target(zerialize_modules)

target_link_libraries(zerialize_modules
    PUBLIC
    zerialize
)

target_include_directories(zerialize_modules
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(zerialize_modules PUBLIC cxx_std_20)