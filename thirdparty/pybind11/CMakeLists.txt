# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.25)

include(FetchContent)

# Tell the user what's happening.
message(STATUS "Building pybind11 as part of the ACTS project")

# Declare where to get pybind from.
FetchContent_Declare(pybind11 SYSTEM ${ACTS_PYBIND11_SOURCE})

# Now set up its build.
set(PYBIND11_TEST OFF)
set(PYBIND11_INSTALL OFF)
FetchContent_MakeAvailable(pybind11)
