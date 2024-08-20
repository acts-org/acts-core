add_library(
    ActsExamplesTruthTracking
    SHARED
    ActsExamples/TruthTracking/ParticleSelector.cpp
    ActsExamples/TruthTracking/ParticleSmearing.cpp
    ActsExamples/TruthTracking/TrackParameterSelector.cpp
    ActsExamples/TruthTracking/TrackModifier.cpp
    ActsExamples/TruthTracking/TrackTruthMatcher.cpp
    ActsExamples/TruthTracking/TruthSeedSelector.cpp
    ActsExamples/TruthTracking/TruthTrackFinder.cpp
    ActsExamples/TruthTracking/TruthVertexFinder.cpp
    ActsExamples/TruthTracking/TruthSeedingAlgorithm.cpp
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
