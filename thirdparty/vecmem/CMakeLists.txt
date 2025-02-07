# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.25)

# CMake include(s).
include(FetchContent)

# Tell the user what's happening.
message(STATUS "Building VecMem as part of the Acts project")

set(VECMEM_VERSION "v${_acts_vecmem_version}")

# Declare where to get VecMem from.
FetchContent_Declare(VecMem SYSTEM ${ACTS_VECMEM_SOURCE})

# Options used in the build of VecMem.
set(VECMEM_BUILD_TESTING
    FALSE
    CACHE BOOL
    "Turn off the build of the VecMem unit tests"
)
set(VECMEM_BUILD_BENCHMARKING
    FALSE
    CACHE BOOL
    "Turn off the build of VecMem benchmarking"
)

# Now set up its build.
FetchContent_MakeAvailable(VecMem)
