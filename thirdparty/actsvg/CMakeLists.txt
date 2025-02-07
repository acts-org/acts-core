# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.25)

# CMake include(s).
include(FetchContent)

# Tell the user what's happening.
message(STATUS "Building actsvg as part of the ACTS project")

FetchContent_Declare(actsvg SYSTEM ${ACTS_ACTSVG_SOURCE})

set(ACTSVG_BUILD_PYTHON_BINDINGS
    OFF
    CACHE BOOL
    "Do not build the Python bindings for actsvg"
)

# Now set up its build.
FetchContent_MakeAvailable(actsvg)
