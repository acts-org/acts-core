add_library(
    ActsExamplesTrackFitting
    SHARED
    src/RefittingCalibrator.cpp
    src/SurfaceSortingAlgorithm.cpp
    src/TrackFittingAlgorithm.cpp
    src/KalmanFitterFunction.cpp
    src/RefittingAlgorithm.cpp
    src/GsfFitterFunction.cpp
    src/GlobalChiSquareFitterFunction.cpp
)
target_include_directories(
    ActsExamplesTrackFitting
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesTrackFitting
    PUBLIC ActsCore ActsExamplesFramework ActsExamplesMagneticField
)

install(
    TARGETS ActsExamplesTrackFitting
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
