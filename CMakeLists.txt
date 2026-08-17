cmake_minimum_required(VERSION 3.18)

# Compute capability of the target GPU. The results in the paper were collected
# on an NVIDIA RTX A4000 (sm_86). Override with, e.g.
#   cmake -DCMAKE_CUDA_ARCHITECTURES=90 ..
# This must precede project(), which otherwise installs its own default.
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
  set(CMAKE_CUDA_ARCHITECTURES 86)
endif()

project(SSTA-X
  VERSION 1.0.0
  DESCRIPTION "GPU-accelerated first-order block-based statistical static timing analysis"
  LANGUAGES CXX CUDA
)

# -----------------------------------------------------------------------------
# Build type
# -----------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type specified; defaulting to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

message(STATUS "CUDA architectures: ${CMAKE_CUDA_ARCHITECTURES}")

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)

# timer.hpp pulls in the CUDA headers, so the plain C++ translation units need
# the toolkit include directories too.
find_package(CUDAToolkit REQUIRED)

set(SSTA_X_PARSER_SPEF ${PROJECT_SOURCE_DIR}/3rd-party/Parser-SPEF)
if(NOT EXISTS ${SSTA_X_PARSER_SPEF}/parser-spef/parser-spef.hpp)
  message(FATAL_ERROR
    "Parser-SPEF not found under 3rd-party/. Fetch the submodule with:\n"
    "  git submodule update --init --recursive")
endif()

# -----------------------------------------------------------------------------
# Kernel tuning knobs
#
# ssta/cuda/tuning_macros.cuh exposes the per-kernel block sizes and
# __launch_bounds__ settings as overridable macros. Pass alternatives with
#   cmake -DSSTA_X_TUNING_DEFS="-DBLOCK_MAX=512;-DBLOCK_DELAY=128" ..
# -----------------------------------------------------------------------------
set(SSTA_X_TUNING_DEFS "" CACHE STRING
  "Semicolon-separated -D flags forwarded to the CUDA kernels")

# -----------------------------------------------------------------------------
# Library
# -----------------------------------------------------------------------------
add_library(ssta-x STATIC
  # circuit and library front end
  ssta/cell/cell.cpp
  ssta/verilog/verilog.cpp
  ssta/unit/unit.cpp
  ssta/utility/os.cpp
  ssta/utility/tokenizer.cpp
  # timing graph construction and host-side orchestration
  ssta/timer/timer.cpp
  ssta/timer/spef.cpp
  ssta/timer/timing.cpp
  ssta/timer/cuda.cpp
  # GPU kernels
  ssta/cuda/rct.cu
  ssta/cuda/timing.cu
)

target_include_directories(ssta-x PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${SSTA_X_PARSER_SPEF}
)

target_compile_options(ssta-x PRIVATE
  $<$<COMPILE_LANGUAGE:CUDA>:${SSTA_X_TUNING_DEFS}>
)

target_link_libraries(ssta-x PUBLIC
  CUDA::cudart
  Threads::Threads
  OpenMP::OpenMP_CXX
)

set_target_properties(ssta-x PROPERTIES
  CUDA_SEPARABLE_COMPILATION OFF
  POSITION_INDEPENDENT_CODE ON
)

# -----------------------------------------------------------------------------
# Command-line driver
# -----------------------------------------------------------------------------
add_executable(ssta-x-bin main/ssta-x.cpp)
set_target_properties(ssta-x-bin PROPERTIES OUTPUT_NAME ssta-x)
target_link_libraries(ssta-x-bin PRIVATE ssta-x)

# -----------------------------------------------------------------------------
# Examples
# -----------------------------------------------------------------------------
option(SSTA_X_BUILD_EXAMPLES "Build the example programs" ON)
if(SSTA_X_BUILD_EXAMPLES)
  add_executable(c17 examples/c17.cpp)
  target_link_libraries(c17 PRIVATE ssta-x)
  set_target_properties(c17 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
endif()
