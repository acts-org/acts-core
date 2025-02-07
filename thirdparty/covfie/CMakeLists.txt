# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.25)

# CMake include(s).
include(FetchContent)

# Tell the user what's happening.
message(STATUS "Building Covfie as part of the Acts project")

set(COVFIE_VERSION "v${_acts_covfie_version}")

# Declare where to get covfie from.
FetchContent_Declare(covfie SYSTEM ${ACTS_COVFIE_SOURCE})

# Options used for covfie.
set(COVFIE_BUILD_EXAMPLES OFF CACHE BOOL "Build covfie examples")
set(COVFIE_BUILD_TESTS OFF CACHE BOOL "Build covfie tests")
set(COVFIE_BUILD_BENCHMARKS OFF CACHE BOOL "Build covfie benchmarks")

set(COVFIE_PLATFORM_CPU ON CACHE BOOL "Enable covfie CPU platform")
set(COVFIE_PLATFORM_CUDA
    ${ACTS_BUILD_PLUGIN_CUDA}
    CACHE BOOL
    "Enable covfie CUDA platform"
)

set(COVFIE_REQUIRE_CXX20 OFF CACHE BOOL "Enable covfie C++20 requirement")
set(COVFIE_QUIET ON CACHE BOOL "Quiet covfie feature warnings")

# Now set up its build.
FetchContent_MakeAvailable(covfie)
