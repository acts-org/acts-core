# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_library(
    ActsExamplesTrackFindingExaTrkX
    SHARED
    src/TrackFindingAlgorithmExaTrkX.cpp
    src/PrototracksToParameters.cpp
    src/TrackFindingFromPrototrackAlgorithm.cpp
    src/TruthGraphBuilder.cpp
    src/createFeatures.cpp
)

target_include_directories(
    ActsExamplesTrackFindingExaTrkX
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
    ActsExamplesTrackFindingExaTrkX
    PUBLIC ActsPluginExaTrkX ActsExamplesFramework ActsExamplesTrackFinding
)

install(
    TARGETS ActsExamplesTrackFindingExaTrkX
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
