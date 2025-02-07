# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(SOURCES
    src/AmbiguityResolutionMLAlgorithm.cpp
    src/SeedFilterMLAlgorithm.cpp
)

add_library(ActsExamplesTrackFindingML SHARED ${SOURCES})

target_include_directories(
    ActsExamplesTrackFindingML
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
    ActsExamplesTrackFindingML
    PUBLIC ActsCore ActsPluginOnnx ActsExamplesFramework
)

install(
    TARGETS ActsExamplesTrackFindingML
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
