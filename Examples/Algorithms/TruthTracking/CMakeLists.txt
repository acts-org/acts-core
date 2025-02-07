# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_library(
    ActsExamplesTruthTracking
    SHARED
    ActsExamples/TruthTracking/ParticleSelector.cpp
    ActsExamples/TruthTracking/ParticleTrackParamExtractor.cpp
    ActsExamples/TruthTracking/TrackParameterSmearing.cpp
    ActsExamples/TruthTracking/TrackParameterSelector.cpp
    ActsExamples/TruthTracking/TrackModifier.cpp
    ActsExamples/TruthTracking/TrackTruthMatcher.cpp
    ActsExamples/TruthTracking/TruthTrackFinder.cpp
    ActsExamples/TruthTracking/TruthVertexFinder.cpp
    ActsExamples/TruthTracking/TruthSeedingAlgorithm.cpp
    ActsExamples/TruthTracking/HitSelector.cpp
)
target_include_directories(
    ActsExamplesTruthTracking
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(
    ActsExamplesTruthTracking
    PUBLIC ActsCore ActsExamplesFramework
)

install(
    TARGETS ActsExamplesTruthTracking
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
