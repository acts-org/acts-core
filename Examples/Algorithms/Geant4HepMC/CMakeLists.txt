add_library(
    ActsExamplesEventRecording
    SHARED
    src/EventAction.cpp
    src/EventRecording.cpp
    src/PrimaryGeneratorAction.cpp
    src/RunAction.cpp
    src/SteppingAction.cpp
)
target_include_directories(
    ActsExamplesEventRecording
    SYSTEM
    PUBLIC ${HEPMC3_INCLUDE_DIR}
)
target_include_directories(
    ActsExamplesEventRecording
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesEventRecording
    PUBLIC ActsCore ActsExamplesFramework ActsExamplesGeant4 ${HEPMC3_LIBRARIES}
)

install(
    TARGETS ActsExamplesEventRecording
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
