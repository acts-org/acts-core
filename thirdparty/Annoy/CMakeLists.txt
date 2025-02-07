# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.25)

# CMake include(s).
include(FetchContent)

# Information message.
message(STATUS "Building Annoy as part of the ACTS project")

# Version of the Annoy library to fetch.
set(ANNOY_VERSION "v${_acts_annoy_version}")

# Declare the fetching of the Annoy library.
FetchContent_Declare(
    Annoy
    SYSTEM
    ${ACTS_ANNOY_SOURCE}
    PATCH_COMMAND
        patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/0001-Modify-annoy.patch
)

# Make the fetched content available.
# Annoy contains a CMakeLists.txt file, so it will be added as a subdirectory.
FetchContent_MakeAvailable(Annoy)
