add_library(
    ActsExamplesIoEDM4hep
    SHARED
    src/EDM4hepMeasurementReader.cpp
    src/EDM4hepMeasurementWriter.cpp
    src/EDM4hepMultiTrajectoryWriter.cpp
    src/EDM4hepReader.cpp
    src/EDM4hepTrackWriter.cpp
    src/EDM4hepTrackReader.cpp
    src/EDM4hepParticleWriter.cpp
    src/EDM4hepSimHitWriter.cpp
    src/EDM4hepUtil.cpp
)
target_include_directories(
    ActsExamplesIoEDM4hep
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesIoEDM4hep
    PUBLIC
        EDM4HEP::edm4hep
        ActsPluginEDM4hep
        podio::podioRootIO
        ActsCore
        ActsFatras
        ActsExamplesFramework
        ActsExamplesDigitization
        ActsExamplesDetectorDD4hep
)

install(
    TARGETS ActsExamplesIoEDM4hep
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
