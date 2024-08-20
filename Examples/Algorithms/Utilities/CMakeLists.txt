add_library(
    ActsExamplesUtilities
    SHARED
    src/PrototracksToSeeds.cpp
    src/SeedsToPrototracks.cpp
    src/TrajectoriesToPrototracks.cpp
    src/TrackSelectorAlgorithm.cpp
    src/TracksToTrajectories.cpp
    src/PrototracksToTracks.cpp
    src/HitSelector.cpp
    src/TracksToParameters.cpp
)
target_include_directories(
    ActsExamplesUtilities
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesUtilities
    PUBLIC ActsCore ActsExamplesFramework
)

install(
    TARGETS ActsExamplesUtilities
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
