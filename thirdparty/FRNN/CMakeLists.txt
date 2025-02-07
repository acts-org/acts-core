# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.25)

include(FetchContent)

message(STATUS "Building FRNN as part of the ACTS project")

# Declare where to get frnncontent from
FetchContent_Declare(frnncontent SYSTEM ${ACTS_FRNN_SOURCE})

# FRNN does not provide a CMakeLists.txt, so we use a custom one. Because of this,
# we have to implement the populate step manually
FetchContent_GetProperties(frnncontent)
if(NOT frnncontent_POPULATED)
    FetchContent_Populate(frnncontent)
    configure_file(
        CMakeLists.txt.in
        "${frnncontent_SOURCE_DIR}/CMakeLists.txt"
        COPYONLY
    )
    add_subdirectory(${frnncontent_SOURCE_DIR} ${frnncontent_BINARY_DIR})
endif()
