add_executable(
  ActsExampleGeant4DD4hep
  DD4hepGeant4Simulation.cpp)
target_link_libraries(
  ActsExampleGeant4DD4hep
  PRIVATE
    ActsExamplesCommon
    ActsExamplesDetectorDD4hep
    ActsExamplesFramework
    ActsExamplesGeant4
    ActsExamplesGeant4Common
    ActsExamplesMagneticField
    ActsExamplesIoRoot
    Boost::program_options)

install(
  TARGETS ActsExampleGeant4DD4hep
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


add_executable(
  ActsExampleMaterialRecordingDD4hep
  DD4hepMaterialRecording.cpp)
target_link_libraries(
  ActsExampleMaterialRecordingDD4hep
  PRIVATE
    ActsExamplesCommon
    ActsExamplesDetectorDD4hep
    ActsExamplesFramework
    ActsExamplesGeant4
    ActsExamplesGeant4Common
    ActsExamplesIoRoot
    Boost::program_options)

install(
  TARGETS ActsExampleMaterialRecordingDD4hep
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
