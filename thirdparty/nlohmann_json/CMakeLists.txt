# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.25)

include(FetchContent)

# Tell the user what's happening.
message(STATUS "Building nlohmann_json as part of the ACTS project")

# Declare where to get nlohmann json from.
FetchContent_Declare(nlohmann_json SYSTEM ${ACTS_NLOHMANNJSON_SOURCE})

# Now set up its build.
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install ON CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)
