# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_library(
    ActsExamplesAlignment
    SHARED
    src/AlignmentAlgorithm.cpp
    src/AlignmentAlgorithmFunction.cpp
)
target_include_directories(
    ActsExamplesAlignment
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesAlignment
    PUBLIC
        ActsCore
        ActsAlignment
        ActsExamplesFramework
        ActsExamplesMagneticField
)

install(
    TARGETS ActsExamplesAlignment
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
